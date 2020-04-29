#ifndef IBUGAVDEFAULTRENDERER_H
#define IBUGAVDEFAULTRENDERER_H

#include "IBugAVRenderer.h"

namespace BugAV {
class IBugAVDefaultRenderer: public IBugAVRenderer
{
public:
    IBugAVDefaultRenderer();
    ~IBugAVDefaultRenderer();

    void updateData(unsigned char **);
    void initShader(int w, int h);    

    void updateData(AVFrame *frame);
};
}
#endif // IBUGAVDEFAULTRENDERER_H
