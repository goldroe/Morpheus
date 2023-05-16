cbuffer CB_Per_Obj {
    float4x4 wvp;
};

Texture2D tex;
SamplerState sample;

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 tex_coord : TEXCOORD0;
};

VS_OUTPUT vs_main(float4 in_pos : POSITION, float2 tex_coord : TEXCOORD0) {
    VS_OUTPUT output;
    output.pos = mul(wvp, in_pos);
    output.tex_coord = tex_coord;
    return output;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET {
    float4 diffuse = tex.Sample(sample, input.tex_coord);
    clip(diffuse.a - .25);
    return diffuse;
}
