/**
 * @file class3/deferred/hazeF.glsl
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

out vec4 frag_color;

uniform sampler2D normalMap;

// Inputs
uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int  sun_up_factor;
in vec2 vary_fragcoord;

vec3 getNorm(vec2 pos_screen);
vec4 getPositionWithDepth(vec2 pos_screen, float depth);
void calcAtmosphericVarsLinear(vec3 inPositionEye, vec3 norm, vec3 light_dir, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive);

float getDepth(vec2 pos_screen);

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

uniform vec4 waterPlane;

uniform int cube_snapshot;

uniform float sky_hdr_scale;

void main()
{
    vec2  tc           = vary_fragcoord.xy;
    float depth        = getDepth(tc.xy);
    vec4  pos          = getPositionWithDepth(tc, depth);
    vec4  norm         = texture(normalMap, tc);
    norm.xyz           = getNorm(tc);
    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;

    vec3  color = vec3(0);
    float bloom = 0.0;

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

    calcAtmosphericVarsLinear(pos.xyz, norm.xyz, light_dir, sunlit, amblit, additive, atten);

    vec3 sunlit_linear = srgb_to_linear(sunlit);
    vec3 amblit_linear = amblit;

    bool do_atmospherics = false;

    // mask off atmospherics below water
    if (dot(pos.xyz, waterPlane.xyz) + waterPlane.w > 0.0)
    {
        do_atmospherics = true;
    }

    vec3  irradiance = vec3(0);
    vec3  radiance  = vec3(0);

    if (GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_PBR))
    {
    }
    else if (!GET_GBUFFER_FLAG(GBUFFER_FLAG_HAS_ATMOS))
    {
        //should only be true of WL sky, just port over base color value
        discard;
    }

   float alpha = 0.0;

    if (do_atmospherics)
    {
        alpha = atten.r;
        color = srgb_to_linear(additive*2.0);
        color *= sky_hdr_scale;
    }
    else
    {
        color = vec3(0,0,0);
        alpha = 1.0;
    }

    frag_color.rgb = max(color.rgb, vec3(0)); //output linear since local lights will be added to this shader's results
    frag_color.a = alpha;
}
