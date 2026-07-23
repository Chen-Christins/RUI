// config.cc
#include "config.hpp"

#include <QIODevice>

namespace rui {

namespace {

QString dottedName(const QString& prefix, const QString& name) {
    if (prefix.isEmpty()) {
        return name;
    }
    return prefix + "." + name;
}

} // namespace

ConfigVarBase::ConfigVarBase(std::string name, std::string description)
    : name_(std::move(name))
    , description_(std::move(description)) {
}

const std::string& ConfigVarBase::name() const {
    return name_;
}

const std::string& ConfigVarBase::description() const {
    return description_;
}

bool ConfigManager::parse(QIODevice* device, QString* error) {
    if (!device) {
        if (error) {
            *error = "config input is null";
        }
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        values_.clear();
    }

    QXmlStreamReader xml(device);
    parseXml(xml);

    if (xml.hasError()) {
        if (error) {
            *error = xml.errorString();
        }
        return false;
    }

    applyValues();
    return true;
}

void ConfigManager::registerVar(const std::shared_ptr<ConfigVarBase>& var) {
    if (!var) {
        return;
    }

    QVariant value;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        vars_[var->name()].push_back(var);
        auto it = values_.find(var->name());
        if (it != values_.end()) {
            value = it->second;
        }
    }

    if (value.isValid()) {
        var->setFromVariant(value);
    }
}

QVariant ConfigManager::getRaw(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(name);
    return it == values_.end() ? QVariant{} : it->second;
}

void ConfigManager::setRaw(const std::string& name, const QVariant& value) {
    std::vector<std::shared_ptr<ConfigVarBase>> vars;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        values_[name] = value;
        auto it = vars_.find(name);
        if (it != vars_.end()) {
            vars = it->second;
        }
    }

    for (const auto& var : vars) {
        var->setFromVariant(value);
    }
}

void ConfigManager::parseXml(QXmlStreamReader& xml) {
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token != QXmlStreamReader::StartElement) {
            continue;
        }

        const QString rootName = xml.name().toString();
        while (!xml.atEnd() && !xml.hasError()) {
            token = xml.readNext();
            if (token == QXmlStreamReader::StartElement) {
                parseElement(xml);
            } else if (token == QXmlStreamReader::EndElement && xml.name() == rootName) {
                break;
            }
        }
        break;
    }
}

void ConfigManager::parseElement(QXmlStreamReader& xml, const QString& prefix) {
    const QString name = xml.name().toString();
    const QString key = dottedName(prefix, name);

    const auto attributes = xml.attributes();
    for (const auto& attribute : attributes) {
        const QString attributeKey = dottedName(key, attribute.name().toString());
        setRaw(attributeKey.toStdString(), attribute.value().toString());
    }

    QString text;
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            parseElement(xml, key);
        } else if (token == QXmlStreamReader::Characters) {
            text += xml.text().toString();
        } else if (token == QXmlStreamReader::EndElement && xml.name() == name) {
            break;
        }
    }

    text = text.trimmed();
    if (!text.isEmpty()) {
        setRaw(key.toStdString(), text);
    }
}

void ConfigManager::applyValues() {
    std::map<std::string, std::vector<std::shared_ptr<ConfigVarBase>>> vars;
    std::map<std::string, QVariant> values;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        vars = vars_;
        values = values_;
    }

    for (const auto& [name, configVars] : vars) {
        auto it = values.find(name);
        if (it != values.end()) {
            for (const auto& var : configVars) {
                var->setFromVariant(it->second);
            }
        }
    }
}

} // namespace rui
