

float4 FillColorPS(in noperspective float4 Position : SV_Position,
                   in noperspective float2 UV : TEXCOORD) : SV_Target0
{
    return float4(1.0, 0.0, 1.0, 1.0);
}