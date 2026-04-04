#include "VersionNotice.h"
#include "glacie/GlacieAPI.h"
#include "gmlib/gm/papi/PlaceholderAPI.h"
#include "ll/api/Config.h"
#include "ll/api/Versions.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/mod/RegisterHelper.h"
#include "resource.h"

namespace gm {

VersionNotice& VersionNotice::getInstance() {
    static VersionNotice instance;
    return instance;
}

bool VersionNotice::load() { return true; }

bool VersionNotice::enable() {
    mConfig.emplace();
    if (!ll::config::loadConfig(*mConfig, getSelf().getConfigDir() / u8"config.json")) {
        ll::config::saveConfig(*mConfig, getSelf().getConfigDir() / u8"config.json");
    }
    mI18n.emplace(getSelf().getLangDir());
    mI18n->updateOrCreateLanguage("en_US", en_US);
    mI18n->updateOrCreateLanguage("zh_CN", zh_CN);
    mI18n->loadAllLanguages();
    mI18n->chooseLanguage(mConfig->language);

    // 新版 PAPI
    auto& papi = gmlib::PlaceholderAPI::getInstance();

    // 注册变量
    if (!papi.getPlaceholderData("player_client_version")) {
        papi.registerPlaceholder(
            "player_client_version",
            [&](optional_ref<gmlib::GMPlayer> player) -> std::optional<std::string> {
                if (player.has_value()) {
                    if (auto version = glacie::getClientVersion(player->getNetworkIdentifier())) {
                        return version->to_string();
                    }
                }
                return {};
            }
        );
    }
    
    auto& eventBus = ll::event::EventBus::getInstance();
    eventBus.emplaceListener<ll::event::PlayerJoinEvent>([&](const ll::event::PlayerJoinEvent& ev) {
            auto& pl = static_cast<gmlib::GMPlayer&>(ev.self());
            auto& papiRef = gmlib::PlaceholderAPI::getInstance();

            auto clientProto = pl.getNetworkProtocolVersion();
            auto serverProto = ll::getNetworkProtocolVersion();

            if (clientProto < serverProto) {
                if (mConfig->send_form) {
                    ll::form::SimpleForm fm("form.title"_tr(), papiRef.translate("form.content"_tr(), pl));
                    fm.appendButton("form.confirm"_tr());
                    fm.sendTo(pl);
                }
                if (mConfig->send_notice) pl.sendMessage(papiRef.translate("notice.message"_tr(), pl));
                if (mConfig->send_toast) pl.sendToast("toast.title"_tr(), papiRef.translate("toast.message"_tr(), pl));
            }

            else if (clientProto > serverProto) {
                if (mConfig->send_form) {
                    ll::form::SimpleForm fm("form.title"_tr(), papiRef.translate("higher_version.form.content"_tr(), pl));
                    fm.appendButton("form.confirm"_tr());
                    fm.sendTo(pl);
                }
                if (mConfig->send_notice) pl.sendMessage(papiRef.translate("higher_version.notice.message"_tr(), pl));
                if (mConfig->send_toast) pl.sendToast("higher_version.toast.title"_tr(), papiRef.translate("higher_version.toast.message"_tr(), pl));
            }
        });
    return true;
}

bool VersionNotice::disable() {
    mI18n.reset();
    mConfig.reset();
    return true;
}

bool VersionNotice::unload() { return true; }

gmlib::i18n::LangI18n& VersionNotice::getI18n() { return *mI18n; }

} // namespace gm

LL_REGISTER_MOD(gm::VersionNotice, gm::VersionNotice::getInstance());