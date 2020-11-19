#ifndef GLOBAL_H
#define GLOBAL_H
namespace BugAV {
enum class AVState {
    StoppedState,
//    LoadingState,
//    LoadedState,
    PlayingState,
    PausedState,
    PlayingNoRenderState,

};

enum class MediaStatus {
    FirstFrameComing,
    NoFrameRenderTooLong,
    NoMoreFrame,
    LoadedState,
    LoadingState,
};

enum class ModePlayer {
    RealTime,
    VOD,
};
}
#endif // GLOBAL_H
