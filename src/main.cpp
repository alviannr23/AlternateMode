#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

static bool isEnabled() {
    return Mod::get()->getSettingValue<bool>("mod-enabled");
}

// indicator setup + track alternating state (reset when death)
class $modify(AlternatePlayLayer, PlayLayer) {
    struct Fields {
        int lastPlayer = 0;
        bool inputLocked = false;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        if (!Mod::get()->getSettingValue<bool>("show-indicator")) return true;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto label = CCLabelBMFont::create("N/A", "bigFont.fnt");
        label->setID("alternate-indicator"_spr);

        float scale = Mod::get()->getSettingValue<double>("indicator-scale");
        float oy = Mod::get()->getSettingValue<double>("indicator-offset-y");

        label->setScale(scale);
        label->setAnchorPoint(ccp(0.5f, 0.f));
        label->setPosition(ccp(winSize.width / 2, 20.f + oy));
        label->setVisible(isEnabled());
        this->addChild(label, 999);
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->lastPlayer = 0;
        m_fields->inputLocked = false;
        auto label = typeinfo_cast<CCLabelBMFont*>(this->getChildByID("alternate-indicator"_spr));
        if (label) {
            label->setString("N/A");
            label->setColor(ccc3(255, 255, 255));
        }
    }
};

// blocks input if same player pressed twice in a row
class $modify(AlternateInput, GJBaseGameLayer) {
    CCLabelBMFont* getIndicator() {
        return typeinfo_cast<CCLabelBMFont*>(this->getChildByID("alternate-indicator"_spr));
    }
    void setIndicatorColor(ccColor3B color) {
        if (auto label = getIndicator()) label->setColor(color);
    }
    void killIfHardMode() {
        if (!Mod::get()->getSettingValue<bool>("hard-mode")) return;
        if (m_player1) this->destroyPlayer(m_player1, nullptr);
    }
    void handleButton(bool down, int button, bool player1) {
        auto play = typeinfo_cast<PlayLayer*>(this);
        if (!play) {
            GJBaseGameLayer::handleButton(down, button, player1);
            return;
        }
        bool enabled = isEnabled();
        auto label = getIndicator();
        if (label) label->setVisible(enabled);
        if (!enabled) {
            GJBaseGameLayer::handleButton(down, button, player1);
            return;
        }

        // cast to modify the class to access m_fields
        auto* mod = static_cast<AlternatePlayLayer*>(play);

        // hold = blocked second input, release = unblock.
        if (!down) {
            mod->m_fields->inputLocked = false;
            setIndicatorColor(ccc3(255, 255, 255));
            GJBaseGameLayer::handleButton(down, button, player1);
            return;
        }
        if (mod->m_fields->inputLocked) {
            setIndicatorColor(ccc3(255, 0, 0));
            killIfHardMode();
            return;
        }
        mod->m_fields->inputLocked = true;

        int cur = player1 ? 1 : 2;

        // same player pressed twice = DIE
        if (cur == mod->m_fields->lastPlayer) {
            setIndicatorColor(ccc3(255, 0, 0));
            killIfHardMode();
            return;
        }

        mod->m_fields->lastPlayer = cur;
        setIndicatorColor(ccc3(0, 255, 0));
        if (label) label->setString(cur == 1 ? "P1" : "P2");
        GJBaseGameLayer::handleButton(down, button, player1);
    }
};