#include "ReUpdateNotice.h"
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

ReUpdateNotice& ReUpdateNotice::getInstance() {
    static ReUpdateNotice instance;
    return instance;
}

bool ReUpdateNotice::load() { return true; }

bool ReUpdateNotice::enable() {
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

        if (pl.getNetworkProtocolVersion() < ll::getNetworkProtocolVersion()) {
            if (mConfig->send_form) {
                std::string title = "form.title"_tr();
                std::string content = papiRef.translate("form.content"_tr(), pl);
                
                ll::form::SimpleForm fm(title, content);
                fm.appendButton("form.confirm"_tr());
                fm.sendTo(pl);
            }
            if (mConfig->send_notice) {
                pl.sendMessage(papiRef.translate("notice.message"_tr(), pl));
            }
            if (mConfig->send_toast) {
                std::string tTitle = "toast.title"_tr();
                std::string tMsg = papiRef.translate("toast.message"_tr(), pl);
                pl.sendToast(tTitle, tMsg);
            }
        }
    });
    return true;
}

bool ReUpdateNotice::disable() {
    mI18n.reset();
    mConfig.reset();
    return true;
}

bool ReUpdateNotice::unload() { return true; }

gmlib::i18n::LangI18n& ReUpdateNotice::getI18n() { return *mI18n; }

} // namespace gm

LL_REGISTER_MOD(gm::ReUpdateNotice, gm::ReUpdateNotice::getInstance());