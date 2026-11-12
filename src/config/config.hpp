// config.hpp
#pragma once

#include <QString>
#include <QVariant>
#include <QXmlStreamReader>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../utils/noncopyable.hpp"
#include "../utils/singleton.hpp"

class QIODevice;

namespace rui {

class ConfigVarBase : public Noncopyable {
public:
    ConfigVarBase(std::string name, std::string description);
    ~ConfigVarBase() override = default;

    const std::string& name() const;
    const std::string& description() const;

    virtual void setFromVariant(const QVariant& value) = 0;

private:
    std::string name_;
    std::string description_;
};

class ConfigManager : public Noncopyable {
public:
    ConfigManager() = default;

    // Parse config content from an already-open device. File discovery and I/O belong to the caller.
    bool parse(QIODevice* device, QString* error = nullptr);

    void registerVar(const std::shared_ptr<ConfigVarBase>& var);

    QVariant getRaw(const std::string& name) const;

    void setRaw(const std::string& name, const QVariant& value);

private:
    void parseXml(QXmlStreamReader& xml);

    void parseElement(QXmlStreamReader& xml, const QString& prefix = {});

    void applyValues() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, QVariant> values_;
    std::map<std::string, std::vector<std::shared_ptr<ConfigVarBase>>> vars_;
};

using ConfigMgr = Singleton<ConfigManager>;

template <typename T>
class ConfigVar : public ConfigVarBase {
    static_assert(!std::is_same_v<T, QString>, "Use ConfigVar<std::string> instead of ConfigVar<QString>.");

public:
    using ptr = std::shared_ptr<ConfigVar<T>>;

    ConfigVar(std::string name, T defaultValue, std::string description)
        : ConfigVarBase(std::move(name), std::move(description))
        , value_(std::move(defaultValue))
    {
    }

    const T& get() const { return value_; }

    operator const T&() const { return value_; }

    void setFromVariant(const QVariant& value) override
    {
        if (!value.isValid()) {
            return;
        }

        if constexpr (std::is_same_v<T, std::string>) {
            value_ = value.toString().toStdString();
        } else if constexpr (std::is_same_v<T, bool>) {
            if (value.typeId() == QMetaType::QString) {
                const QString text = value.toString().trimmed().toLower();
                value_ = text == "true" || text == "1" || text == "yes" || text == "on";
            } else {
                value_ = value.toBool();
            }
        } else if constexpr (std::is_integral_v<T>) {
            value_ = static_cast<T>(value.toLongLong());
        } else if constexpr (std::is_floating_point_v<T>) {
            value_ = static_cast<T>(value.toDouble());
        } else {
            value_ = value.value<T>();
        }
    }

    static ConfigVar::ptr Lookup(const std::string& name, T defaultValue, const std::string& description = {})
    {
        auto var = std::make_shared<ConfigVar<T>>(name, std::move(defaultValue), description);
        ConfigMgr::Instance()->registerVar(var);
        return var;
    }

private:
    T value_;
};

} // namespace rui
