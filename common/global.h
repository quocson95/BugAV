#ifndef GLOBAL_H
#define GLOBAL_H
namespace BugAV {
enum class AVState {
    StoppedState,
    LoadingState,
    PlayingState,
    PausedState,
    PlayingNoRenderState,

};

enum class MediaStatus {
    FirstFrameComing,
    NoFrameRenderTooLong,
};

enum class ModePlayer {
    RealTime,
    VOD,
};
}
#endif // GLOBAL_H
