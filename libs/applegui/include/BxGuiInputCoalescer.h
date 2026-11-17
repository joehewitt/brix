#ifndef BX_GUI_INPUT_COALESCER_H
#define BX_GUI_INPUT_COALESCER_H

#include <stdint.h>

#include <string>

enum class BxGuiInputKind {
    Barrier,
    PointerMove,
    PointerDrag,
    Scroll,
    Magnify,
    DropOver,
    TextChange
};

struct BxGuiInputEvent {
    explicit BxGuiInputEvent(BxGuiInputKind anInputKind, uintptr_t aSource = 0):
        inputKind(anInputKind),
        source(aSource),
        eventKind(0),
        pointerId(0),
        deviceKind(0),
        button(0),
        buttons(0),
        modifiers(0),
        x(0),
        y(0),
        deltaX(0),
        deltaY(0),
        orderingTime(0),
        clickCount(0),
        selectionStart(0),
        selectionEnd(0),
        isPrecise(false),
        phase(0),
        momentumPhase(0),
        constituentCount(1)
    {}

    bool merge(const BxGuiInputEvent& newer) {
        if (inputKind != newer.inputKind || source != newer.source) {
            return false;
        }

        switch (inputKind) {
            case BxGuiInputKind::PointerMove:
            case BxGuiInputKind::PointerDrag:
                if (pointerId != newer.pointerId || deviceKind != newer.deviceKind) {
                    return false;
                }
                deltaX += newer.deltaX;
                deltaY += newer.deltaY;
                copyNewestPointerState(newer);
                break;

            case BxGuiInputKind::Scroll:
                if (pointerId != newer.pointerId ||
                    deviceKind != newer.deviceKind ||
                    isPrecise != newer.isPrecise ||
                    phase != newer.phase ||
                    momentumPhase != newer.momentumPhase ||
                    isScrollTransition()) {
                    return false;
                }
                deltaX += newer.deltaX;
                deltaY += newer.deltaY;
                copyNewestPointerState(newer);
                break;

            case BxGuiInputKind::Magnify:
                if (pointerId != newer.pointerId ||
                    deviceKind != newer.deviceKind ||
                    phase != newer.phase ||
                    isScrollTransition()) {
                    return false;
                }
                deltaX = (1 + deltaX) * (1 + newer.deltaX) - 1;
                copyNewestPointerState(newer);
                break;

            case BxGuiInputKind::DropOver:
                if (pointerId != newer.pointerId) {
                    return false;
                }
                copyNewestPointerState(newer);
                break;

            case BxGuiInputKind::TextChange:
                text = newer.text;
                selectionStart = newer.selectionStart;
                selectionEnd = newer.selectionEnd;
                eventKind = eventKind == 0 || newer.eventKind == 0 ? 0 : newer.eventKind;
                break;

            case BxGuiInputKind::Barrier:
                return false;
        }

        constituentCount += newer.constituentCount;
        return true;
    }

    bool isScrollTransition() const {
        const uint64_t transitionPhases =
            uint64_t{1} |  // began
            uint64_t{8} |  // ended
            uint64_t{16} | // cancelled
            uint64_t{32};  // may begin
        return (phase & transitionPhases) || (momentumPhase & transitionPhases);
    }

    BxGuiInputKind inputKind;
    uintptr_t source;
    uint32_t eventKind;
    uint64_t pointerId;
    uint32_t deviceKind;
    uint32_t button;
    uint64_t buttons;
    uint64_t modifiers;
    double x;
    double y;
    double deltaX;
    double deltaY;
    double orderingTime;
    uint32_t clickCount;
    uint32_t selectionStart;
    uint32_t selectionEnd;
    bool isPrecise;
    uint64_t phase;
    uint64_t momentumPhase;
    std::string text;
    // A future getCoalescedEvents-style list can replace this count without changing stream
    // ownership: every constituent is already merged into one explicit event record.
    uint64_t constituentCount;

private:
    void copyNewestPointerState(const BxGuiInputEvent& newer) {
        eventKind = newer.eventKind;
        pointerId = newer.pointerId;
        deviceKind = newer.deviceKind;
        button = newer.button;
        buttons = newer.buttons;
        modifiers = newer.modifiers;
        x = newer.x;
        y = newer.y;
        orderingTime = newer.orderingTime;
        clickCount = newer.clickCount;
        isPrecise = newer.isPrecise;
        phase = newer.phase;
        momentumPhase = newer.momentumPhase;
    }
};

class BxGuiInputCoalescer {
public:
    BxGuiInputCoalescer(): newest(nullptr) {}

    bool tryMerge(const BxGuiInputEvent& event) {
        return newest && newest->merge(event);
    }

    void append(BxGuiInputEvent& event) {
        newest = &event;
    }

    void begin(BxGuiInputEvent& event) {
        if (newest == &event) {
            newest = nullptr;
        }
    }

    void abandon(BxGuiInputEvent& event) {
        begin(event);
    }

    void seal() {
        newest = nullptr;
    }

    bool hasNewest() const {
        return newest != nullptr;
    }

private:
    BxGuiInputEvent* newest;
};

#endif
