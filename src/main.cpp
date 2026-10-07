#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <cmath>
#include <cfloat>
#include <vector>
#include <algorithm>

using namespace geode::prelude;

enum class FramePerfectEvent {
    None,
    CubeJump,
    ShipInput,
    WaveInput,
    BallInput,
    RobotInput,
    SpiderInput,
    SwingInput,
    YellowOrb,
    PinkOrb,
    BlueOrb,
    RedOrb,
    BlackOrb,
    GreenOrb,
    DashOrb,
    UnknownInput
};

struct DetectedEvent {
    FramePerfectEvent type = FramePerfectEvent::None;
    bool recognized = false;
    bool pressed = false;
    double physicsFrame = 0.0;
};

static FramePerfectEvent getCurrentEventType(PlayerObject* player) {
    if (player->m_isShip)
        return FramePerfectEvent::ShipInput;

    if (player->m_isDart)
        return FramePerfectEvent::WaveInput;

    if (player->m_isBall)
        return FramePerfectEvent::BallInput;

    if (player->m_isRobot)
        return FramePerfectEvent::RobotInput;

    if (player->m_isSpider)
        return FramePerfectEvent::SpiderInput;

    if (player->m_isSwing)
        return FramePerfectEvent::SwingInput;

    if (!player->m_isDead)
        return FramePerfectEvent::CubeJump;

    return FramePerfectEvent::UnknownInput;
}

static const char* eventTypeName(FramePerfectEvent type) {
    switch (type) {
        case FramePerfectEvent::CubeJump:
            return "Cube Jump";

        case FramePerfectEvent::ShipInput:
            return "Ship";

        case FramePerfectEvent::WaveInput:
            return "Wave";

        case FramePerfectEvent::BallInput:
            return "Ball";

        case FramePerfectEvent::RobotInput:
            return "Robot";

        case FramePerfectEvent::SpiderInput:
            return "Spider";

        case FramePerfectEvent::SwingInput:
            return "Swing";

        case FramePerfectEvent::YellowOrb:
            return "Yellow Orb";

        case FramePerfectEvent::PinkOrb:
            return "Pink Orb";

        case FramePerfectEvent::BlueOrb:
            return "Blue Orb";

        case FramePerfectEvent::RedOrb:
            return "Red Orb";

        case FramePerfectEvent::BlackOrb:
            return "Black Orb";

        case FramePerfectEvent::GreenOrb:
            return "Green Orb";

        case FramePerfectEvent::DashOrb:
            return "Dash Orb";

        case FramePerfectEvent::UnknownInput:
            return "Unknown";

        default:
            return "None";
    }
}

static FramePerfectEvent getOrbEventType(RingObject* ring) {
    if (!ring)
        return FramePerfectEvent::UnknownInput;

    switch (ring->m_objectID) {
        case 36:
            return FramePerfectEvent::YellowOrb;

        case 141:
            return FramePerfectEvent::PinkOrb;

        case 84:
            return FramePerfectEvent::BlueOrb;

        case 1333:
            return FramePerfectEvent::RedOrb;

        case 1022:
            return FramePerfectEvent::GreenOrb;

        case 1330:
            return FramePerfectEvent::BlackOrb;

        case 1704:
        case 1751:
            return FramePerfectEvent::DashOrb;

        default:
            return FramePerfectEvent::UnknownInput;
    }
}

static bool isOrbEnabled(FramePerfectEvent type) {
    switch (type) {
        case FramePerfectEvent::YellowOrb:
            return Mod::get()->getSettingValue<bool>("count-yellow-orb");

        case FramePerfectEvent::PinkOrb:
            return Mod::get()->getSettingValue<bool>("count-pink-orb");

        case FramePerfectEvent::BlueOrb:
            return Mod::get()->getSettingValue<bool>("count-blue-orb");

        case FramePerfectEvent::RedOrb:
            return Mod::get()->getSettingValue<bool>("count-red-orb");

        case FramePerfectEvent::BlackOrb:
            return Mod::get()->getSettingValue<bool>("count-black-orb");

        case FramePerfectEvent::GreenOrb:
            return Mod::get()->getSettingValue<bool>("count-green-orb");

        case FramePerfectEvent::DashOrb:
            return Mod::get()->getSettingValue<bool>("count-dash-orb");

        case FramePerfectEvent::CubeJump:
            return Mod::get()->getSettingValue<bool>("count-cube-jump");

        case FramePerfectEvent::ShipInput:
            return Mod::get()->getSettingValue<bool>("count-ship");

        case FramePerfectEvent::WaveInput:
            return Mod::get()->getSettingValue<bool>("count-wave");

        case FramePerfectEvent::RobotInput:
            return Mod::get()->getSettingValue<bool>("count-robot");

        case FramePerfectEvent::SpiderInput:
            return Mod::get()->getSettingValue<bool>("count-spider");

        default:
            return false;
    }
}

static RingObject* findNextOrb(PlayerObject* player) {
    auto playLayer = PlayLayer::get();

    if (!playLayer || !playLayer->m_objects)
        return nullptr;

    RingObject* bestOrb = nullptr;
    float bestDistance = FLT_MAX;

    CCPoint playerPos = player->getPosition();

    unsigned int objectCount =
        playLayer->m_objects->count();

    for (unsigned int i = 0; i < objectCount; i++) {
        auto object =
            static_cast<GameObject*>(
                playLayer->m_objects->objectAtIndex(i)
            );

        if (!object)
            continue;

        auto ring =
            typeinfo_cast<RingObject*>(object);

        if (!ring)
            continue;

        auto type =
            getOrbEventType(ring);

        if (type == FramePerfectEvent::UnknownInput)
            continue;

        if (!isOrbEnabled(type))
            continue;

        float dx =
            ring->getPositionX() -
            playerPos.x;

        float dy =
            ring->getPositionY() -
            playerPos.y;

        if (dx < -50.0f)
            continue;

        if (dx > 3000.0f)
            continue;

        float distance =
            std::sqrt(
                dx * dx +
                dy * dy
            );

        if (distance < bestDistance) {
            bestDistance = distance;
            bestOrb = ring;
        }
    }

    return bestOrb;
}

class $modify(FrameWindowPlayer, PlayerObject) {
    struct Fields {
        PlayerObject* predictor = nullptr;

        CCDrawNode* trajectoryDraw = nullptr;

        int frameCounter = 0;

        double physicsFrame = 0.0;
        double physicsTime = 0.0;

        DetectedEvent lastEvent;

        RingObject* lastRing = nullptr;

        /*
         * Predictor state.
         */
        bool predictionMode = false;

        RingObject* predictionTarget = nullptr;

        bool predictionHit = false;

        int predictionInputFrame = -1;

        CCPoint predictionInputPosition = CCPointZero;

        /*
         * Results from the latest prediction.
         */
        std::vector<int> successfulFrames;

        std::vector<CCPoint> successfulPositions;

        int predictedFirstFrame = -1;
        int predictedLastFrame = -1;

        CCPoint predictedClosestPoint = CCPointZero;

        float predictedClosestDistance = FLT_MAX;
    };

    void update(float dt) {
        PlayerObject::update(dt);

        auto& fields = m_fields;

        int simulationFPS =
            Mod::get()->getSettingValue<int>(
                "simulation-fps"
            );

        if (simulationFPS != 60 &&
            simulationFPS != 120 &&
            simulationFPS != 240) {

            simulationFPS = 60;
        }

        fields->physicsFrame +=
            static_cast<double>(dt) *
            static_cast<double>(simulationFPS);

        fields->physicsTime +=
            static_cast<double>(dt);

        /*
         * Create the hidden simulation player.
         */
        if (!fields->predictor &&
            m_gameLayer &&
            m_mainLayer) {

            auto mainLayer =
                typeinfo_cast<CCLayer*>(m_mainLayer);

            if (!mainLayer)
                return;

            fields->predictor =
                PlayerObject::create(
                    1,
                    0,
                    m_gameLayer,
                    mainLayer,
                    true
                );

            if (fields->predictor) {
                fields->predictor->setVisible(false);
                fields->predictor->copyAttributes(this);
            }
        }

        if (!fields->predictor)
            return;

        /*
         * Temporary trajectory visualization.
         *
         * This is still only a debugging visualization.
         * The final circle comes later.
         */
        if (!fields->trajectoryDraw &&
            m_gameLayer) {

            fields->trajectoryDraw =
                CCDrawNode::create();

            m_gameLayer->addChild(
                fields->trajectoryDraw,
                1000
            );
        }

        if (!fields->trajectoryDraw)
            return;

        /*
         * Don't run the expensive predictor every game tick.
         */
        fields->frameCounter++;

        if (fields->frameCounter % 3 != 0)
            return;

        fields->trajectoryDraw->clear();

        /*
         * Find the next relevant orb from the REAL player.
         */
        RingObject* target =
            findNextOrb(this);

        if (!target)
            return;

        fields->predictionTarget = target;

        float physicsDt =
            1.0f /
            static_cast<float>(simulationFPS);

        /*
         * ---------------------------------------------------------
         * PHASE 1
         *
         * Predict the player's trajectory WITHOUT INPUT.
         *
         * This is important because we first want to know where
         * the player is naturally going before branching possible
         * inputs from that trajectory.
         * ---------------------------------------------------------
         */

        fields->predictor->copyAttributes(this);
        fields->predictor->resetTouchedRings(true);

        CCPoint previousPoint =
            fields->predictor->getPosition();

        int closestFrame = -1;

        float closestDistance = FLT_MAX;

        CCPoint closestPoint =
            CCPointZero;

        /*
         * We simulate up to roughly two seconds.
         *
         * The target is normally encountered much earlier, so we
         * stop once the predictor has passed it.
         */
        int maximumPredictionFrames =
            simulationFPS * 2;

        std::vector<CCPoint> baselinePositions;
        baselinePositions.reserve(maximumPredictionFrames);

        int targetFrame = -1;

        for (int frame = 0;
             frame < maximumPredictionFrames;
             frame++) {

            fields->predictor->updateMove(
                physicsDt
            );

            CCPoint currentPoint =
                fields->predictor->getPosition();

            baselinePositions.push_back(
                currentPoint
            );

            float dx =
                currentPoint.x -
                target->getPositionX();

            float dy =
                currentPoint.y -
                target->getPositionY();

            float distance =
                std::sqrt(
                    dx * dx +
                    dy * dy
                );

            if (distance < closestDistance) {
                closestDistance = distance;
                closestFrame = frame;
                closestPoint = currentPoint;
            }

            /*
             * Draw the baseline trajectory.
             */
            int red =
                Mod::get()->getSettingValue<int>(
                    "trajectory-red"
                );

            int green =
                Mod::get()->getSettingValue<int>(
                    "trajectory-green"
                );

            int blue =
                Mod::get()->getSettingValue<int>(
                    "trajectory-blue"
                );

            red = std::clamp(red, 0, 255);
            green = std::clamp(green, 0, 255);
            blue = std::clamp(blue, 0, 255);

            ccColor4F trajectoryColor =
                ccc4f(
                    red / 255.0f,
                    green / 255.0f,
                    blue / 255.0f,
                    0.75f
                );

            fields->trajectoryDraw->drawSegment(
                previousPoint,
                currentPoint,
                1.5f,
                trajectoryColor
            );

            fields->trajectoryDraw->drawDot(
                currentPoint,
                2.0f,
                trajectoryColor
            );

            previousPoint =
                currentPoint;

            /*
             * Once we've passed the orb horizontally, there is no
             * reason to keep extending the baseline search.
             */
            if (currentPoint.x >
                target->getPositionX() + 100.0f) {

                targetFrame = frame;
                break;
            }
        }

        if (targetFrame < 0)
            targetFrame = closestFrame;

        if (targetFrame < 0)
            return;

        fields->predictedClosestPoint =
            closestPoint;

        fields->predictedClosestDistance =
            closestDistance;

        /*
         * ---------------------------------------------------------
         * PHASE 2
         *
         * Test hypothetical input frames.
         *
         * We test a window around the naturally predicted contact
         * frame rather than blindly simulating thousands of
         * candidates.
         * ---------------------------------------------------------
         */

        fields->successfulFrames.clear();
        fields->successfulPositions.clear();

        /*
         * Start somewhat before the predicted contact point.
         *
         * This will later be expanded/tuned once we verify the
         * simulator's timing against actual gameplay.
         */
        int candidateStart =
            std::max(
                0,
                targetFrame - simulationFPS / 2
            );

        int candidateEnd =
            std::min(
                targetFrame + simulationFPS / 2,
                maximumPredictionFrames - 1
            );

        /*
         * Every candidate is an independent simulation.
         */
        for (int candidate = candidateStart;
             candidate <= candidateEnd;
             candidate++) {

            /*
             * Reset the hidden player to the REAL player's exact
             * current state.
             */
            fields->predictor->copyAttributes(this);
            fields->predictor->resetTouchedRings(true);

            fields->predictionMode = true;
            fields->predictionTarget = target;
            fields->predictionHit = false;
            fields->predictionInputFrame = candidate;
            fields->predictionInputPosition =
                CCPointZero;

            /*
             * Advance until the candidate input frame.
             */
            for (int frame = 0;
                 frame < candidate;
                 frame++) {

                fields->predictor->updateMove(
                    physicsDt
                );

                if (fields->predictionHit)
                    break;
            }

            if (fields->predictionHit) {
                /*
                 * Something already activated the target before
                 * our hypothetical click. This candidate isn't a
                 * valid click timing, so ignore it.
                 */
                fields->predictionHit = false;
            }

            /*
             * Save the player's position AT THE HYPOTHETICAL CLICK.
             */
            CCPoint inputPosition =
                fields->predictor->getPosition();

            /*
             * Inject the hypothetical Jump input.
             *
             * predictionMode prevents this from generating a
             * notification.
             */
            fields->predictor->pushButton(
                PlayerButton::Jump
            );

            /*
             * Continue simulation after the click.
             *
             * ringJump() will set predictionHit when the target
             * orb actually activates.
             */
            int postInputFrames =
                simulationFPS / 2;

            for (int frame = 0;
                 frame < postInputFrames;
                 frame++) {

                fields->predictor->updateMove(
                    physicsDt
                );

                if (fields->predictionHit)
                    break;
            }

            /*
             * The candidate successfully activated THIS target.
             */
            if (fields->predictionHit) {
                fields->successfulFrames.push_back(
                    candidate
                );

                fields->successfulPositions.push_back(
                    inputPosition
                );
            }
        }

        fields->predictionMode = false;
        fields->predictionTarget = nullptr;

        /*
         * Convert successful candidate frames into one continuous
         * frame window.
         */
        if (!fields->successfulFrames.empty()) {

            fields->predictedFirstFrame =
                fields->successfulFrames.front();

            fields->predictedLastFrame =
                fields->successfulFrames.back();

            log::info(
                "PREDICTOR | target={} | window={}..{} | size={}",
                target->m_objectID,
                fields->predictedFirstFrame,
                fields->predictedLastFrame,
                fields->successfulFrames.size()
            );
        }
        else {
            fields->predictedFirstFrame = -1;
            fields->predictedLastFrame = -1;

            log::info(
                "PREDICTOR | target={} | NO VALID INPUT",
                target->m_objectID
            );
        }

        /*
         * Clear predictor-only state.
         */
        fields->predictionInputFrame = -1;
        fields->predictionInputPosition =
            CCPointZero;
    }

    bool pushButton(PlayerButton button) {
        /*
         * This is the critical distinction:
         *
         * predictor clicks are NOT real player clicks.
         */
        if (m_fields->predictionMode) {
            return PlayerObject::pushButton(button);
        }

        bool result =
            PlayerObject::pushButton(button);

        if (m_isDead)
            return result;

        if (button != PlayerButton::Jump)
            return result;

        auto& fields = m_fields;

        /*
         * We only report the click itself here.
         *
         * Orb activation is still detected by ringJump().
         */
        if (fields->lastRing) {

            auto orbType =
                getOrbEventType(
                    fields->lastRing
                );

            if (orbType !=
                FramePerfectEvent::UnknownInput) {

                if (!isOrbEnabled(orbType))
                    return result;

                int windowSize = 0;

                if (fields->predictedFirstFrame >= 0 &&
                    fields->predictedLastFrame >=
                        fields->predictedFirstFrame) {

                    windowSize =
                        fields->predictedLastFrame -
                        fields->predictedFirstFrame +
                        1;
                }

                Notification::create(
                    fmt::format(
                        "CLICK | {} | WINDOW {}",
                        eventTypeName(orbType),
                        windowSize
                    )
                )->show();

                log::info(
                    "REAL CLICK | {} | window={}..{} | size={}",
                    eventTypeName(orbType),
                    fields->predictedFirstFrame,
                    fields->predictedLastFrame,
                    windowSize
                );

                fields->lastEvent.type =
                    orbType;

                fields->lastEvent.recognized =
                    true;

                fields->lastEvent.pressed =
                    true;

                fields->lastEvent.physicsFrame =
                    fields->physicsFrame;
            }
        }

        return result;
    }

    bool releaseButton(PlayerButton button) {
        bool result =
            PlayerObject::releaseButton(button);

        if (m_fields->predictionMode)
            return result;

        if (m_isDead)
            return result;

        if (button != PlayerButton::Jump)
            return result;

        auto& fields = m_fields;

        auto eventType =
            getCurrentEventType(this);

        bool recognized =
            eventType !=
            FramePerfectEvent::UnknownInput;

        fields->lastEvent.type =
            eventType;

        fields->lastEvent.recognized =
            recognized;

        fields->lastEvent.pressed =
            false;

        fields->lastEvent.physicsFrame =
            fields->physicsFrame;

        log::info(
            "TIMING | RELEASE | {} | frame {:.4f} | recognized={}",
            eventTypeName(eventType),
            fields->physicsFrame,
            recognized
        );

        return result;
    }

    void ringJump(
        RingObject* object,
        bool skipCheck
    ) {
        auto& fields = m_fields;

        /*
         * Predictor-only orb detection.
         *
         * This is the important part: the predictor is allowed to
         * call ringJump(), but it cannot create a real notification.
         */
        if (fields->predictionMode &&
            object &&
            fields->predictionTarget == object) {

            fields->predictionHit = true;
        }

        /*
         * Only the REAL player records lastRing.
         *
         * This prevents the hidden predictor from overwriting the
         * real player's orb state.
         */
        if (!fields->predictionMode &&
            object) {

            fields->lastRing =
                object;

            log::info(
                "RING PROCESS | objectID={} | position=({}, {})",
                object->m_objectID,
                object->getPositionX(),
                object->getPositionY()
            );
        }

        PlayerObject::ringJump(
            object,
            skipCheck
        );
    }
};