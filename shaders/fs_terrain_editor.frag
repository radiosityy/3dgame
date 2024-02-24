#version 450
#include "fs_common.h"

void main()
{
    if(common_buf.cur_terrain_intersection != 0)
    {
        const float r = distance(world_pos_in, common_buf.cur_pos_terrain);

        if((r >= common_buf.editor_terrain_tool_inner_radius) && (r <= common_buf.editor_terrain_tool_outer_radius))
        {
            out_col = vec4(0,0,1,1);
            return;
        }
    }

    shadeDefault();
}
