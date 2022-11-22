#ifndef _RENDER_H_
#define _RENDER_H_

#include"glad/glad.h"
#include"types.h"

namespace Render {

bool init(GLADloadproc gl_get_proc_address, u32 width, u32 height);

} // namespace Render

#endif // _RENDER_H_