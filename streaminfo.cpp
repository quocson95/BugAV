#include "streaminfo.h"

StreamInfo::StreamInfo()
    :avctx{nullptr}
    ,codec{nullptr}
    ,streamLowres{0}
    ,index{-1}
    ,stream{nullptr}
    ,decoder{nullptr}
{

}
