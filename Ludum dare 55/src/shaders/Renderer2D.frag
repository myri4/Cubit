#pragma shader_stage(fragment)
#extension GL_EXT_nonuniform_qualifier : require

layout(binding = 1) uniform sampler2D u_Textures[];

layout(location = 0) in vec2 v_TexCoords;
layout(location = 1) in flat uint v_TexID;
layout(location = 2) in vec4 v_Color;
layout(location = 3) in float v_Fade;
layout(location = 4) in float v_Thickness;

layout(location = 0) out vec4 FragColor;

const float pxRange = 2.f;

float median(float r, float g, float b) 
{
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(sampler2D msdf) 
{
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(msdf, 0));
    vec2 screenTexSize = vec2(1.f) / fwidth(v_TexCoords);
    return max(0.5f * dot(unitRange, screenTexSize), 1.f);
}

void main() 
{
    vec4 color = texture(u_Textures[v_TexID], v_TexCoords) * v_Color;

    FragColor = color;
    if (v_Thickness > 0.f) 
    {        
        float dist = 1.f - length(v_TexCoords);
        float alpha = smoothstep(0.f, v_Fade, dist) * smoothstep(v_Thickness + v_Fade, v_Thickness, dist);
        
        FragColor = v_Color;
        FragColor.a = alpha * v_Color.a;
    }
    else if (v_Thickness < 0.f)
    {
        vec3 msd = texture(u_Textures[v_TexID], v_TexCoords).rgb;
        float sd = median(msd.r, msd.g, msd.b);
        float screenPxDistance = screenPxRange(u_Textures[v_TexID]) * (sd - 0.5f);
        float opacity = clamp(screenPxDistance + 0.5f, 0.f, 1.f);

        if (opacity == 0.f) discard;

        FragColor = mix(vec4(0.f), v_Color, opacity);
    }

    if (v_TexID != 0 && v_Thickness >= 0.f) FragColor.rgb *= 0.3f;
}