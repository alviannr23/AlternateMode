#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

// STATE DATA
struct State {
    bool hasPressed = false;
    int lastPlayer = 0;
    bool inputLocked = false;
};

static std::unordered_map<GJBaseGameLayer*, State> states;

// INPUT LOGIC
class $modify(AlternateModeInput, GJBaseGameLayer) {

    CCLabelBMFont* getOverlay() {
        return typeinfo_cast<CCLabelBMFont*>(
            this->getChildByID("alternate-overlay")
        );
    }

    void setOverlayColor(ccColor3B color) {
        if (auto label = getOverlay())
            label->setColor(color);
    }

    void setOverlayText(const char* text) {
        if (auto label = getOverlay())
            label->setString(text);
    }

    void setOverlayVisible(bool v) {
        if (auto label = getOverlay())
            label->setVisible(v);
    }

    void killIfHardMode() {
        if (!Mod::get()->getSettingValue<bool>("hard-mode"))
            return;

        if (m_player1)
            this->destroyPlayer(m_player1, nullptr);
    }

    void handleButton(bool down, int button, bool player1) {
        auto& st = states[this];

        bool enabled = Mod::get()->getSettingValue<bool>("enabled");

        // OVERLAY VISIBLE
        setOverlayVisible(enabled);

        if (!enabled) {
            GJBaseGameLayer::handleButton(down, button, player1);
            return;
        }

        int cur = player1 ? 1 : 2;

        // DOUBLE INPUT CHECK
        if (down) {
            if (st.inputLocked) {
                setOverlayColor(ccc3(255, 0, 0));
                killIfHardMode();
                return;
            }
            st.inputLocked = true;
        } else {
            st.inputLocked = false;
        }

        // RESET COLOR ON RELEASE
        if (!down) {
            setOverlayColor(ccc3(255, 255, 255));
            GJBaseGameLayer::handleButton(down, button, player1);
            return;
        }

        // FIRST INPUT
        if (!st.hasPressed) {
            st.hasPressed = true;
            st.lastPlayer = cur;
            setOverlayColor(ccc3(0, 255, 0));
            setOverlayText(cur == 1 ? "P1" : "P2");
            GJBaseGameLayer::handleButton(down, button, player1);
            return;
        }

        // SAME PLAYER TWICE
        if (cur == st.lastPlayer) {
            setOverlayColor(ccc3(255, 0, 0));
            killIfHardMode();
            return;
        }

        st.lastPlayer = cur;

        setOverlayColor(ccc3(0, 255, 0));
        setOverlayText(cur == 1 ? "P1" : "P2");
        GJBaseGameLayer::handleButton(down, button, player1);
    }
};

// OVERLAY
class $modify(AlternateModePlayLayer, PlayLayer) {

    ~AlternateModePlayLayer() {
        states.erase(this);
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        auto& st = states[this];
        st.hasPressed = false;
        st.lastPlayer = 0;
        st.inputLocked = false;

        auto enabled = Mod::get()->getSettingValue<bool>("enabled");
        auto show = Mod::get()->getSettingValue<bool>("show-indicator");

        if (!show)
            return true;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto label = CCLabelBMFont::create("N/A", "bigFont.fnt");
        if (!label)
            return true;

        label->setID("alternate-overlay");

        float scale = Mod::get()->getSettingValue<double>("overlay-scale");
        float oy    = Mod::get()->getSettingValue<double>("overlay-offset-y");

        if (scale == 1.0f)
            scale = 0.5f;

        label->setScale(scale);
        label->setColor(ccc3(255, 255, 255));
        label->setAnchorPoint(ccp(0.5f, 0.f));
        label->setPosition(ccp(winSize.width / 2, 20.f + oy));

        // HIDDEN LABEL
        label->setVisible(enabled);
        this->addChild(label, 999);

        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        auto& st = states[this];
        st.hasPressed = false;
        st.lastPlayer = 0;
        st.inputLocked = false;
        auto label = typeinfo_cast<CCLabelBMFont*>(
            this->getChildByID("alternate-overlay")
        );

        if (label) {
            label->setString("N/A");
            label->setColor(ccc3(255, 255, 255));
        }
    }
};