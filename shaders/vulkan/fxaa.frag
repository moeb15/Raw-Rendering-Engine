#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform constants{
    uint drawImageIndex;
} PushConstants;

const float EDGE_THRESHOLD_MIN = (1.0 / 16.0);
const float EDGE_THRESHOLD_MAX = (1.0 / 8.0);
const float PIXEL_BLEND_LIMIT_TO_REDUCE_BLURRING = (3.0 / 4.0);
const float MIN_PIXEL_ALIASING_REQUIRED = (1.0 / 8.0);
const float NUM_LOOP_FOR_EDGE_DETECTION = 1;
const int Center = 0;
const int Top = 1;
const int Bottom = 2;
const int Left = 3;
const int Right = 4;
const int TopRight = 5;
const int BottomRight = 6;
const int TopLeft = 7;
const int BottomLeft = 8;
vec2 offsets[] = {
    vec2( 0, 0), vec2( 0, -1), vec2( 0, 1),
    vec2(-1, 0), vec2( 1, 0), vec2( 1, -1),
    vec2( 1, 1), vec2(-1, -1), vec2(-1, 1)};

float rgb2luma(vec3 rgb)
{
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

float findEndPointPosition(vec2 textureCoordMiddle, 
                            float lumaMiddle,
                            float lumaHighContrastPixel,
                            float stepLength,
                            vec2 viewportSizeInverse,
                            bool isHorizontal,
                            out vec2 outPosToFetchTexelForEdgeAntiAliasing);


void main()
{
    ivec2 textureRes = textureSize(globalTextures[nonuniformEXT(PushConstants.drawImageIndex)], 0).xy;

    const vec2 viewportSizeInverse = vec2(1.0 / textureRes.x, 1.0 / textureRes.y);
    const vec2 texCoord = gl_FragCoord.xy * viewportSizeInverse;
    float minLuma = 100000000;
    float maxLuma = 0;
    float lumas[9];
    vec3 rgb[9];
    vec3 rgbSum = vec3(0, 0, 0);

    for(int i = 0; i < 9; i++)
    {
        rgb[i] = texture(globalTextures[nonuniformEXT(PushConstants.drawImageIndex)], texCoord + offsets[i] * viewportSizeInverse).rgb;
        rgbSum += rgb[i];
        lumas[i] = rgb2luma(rgb[i]);
        if(i < 5)
        {
            minLuma = min(lumas[i], minLuma);
            maxLuma = max(lumas[i], maxLuma);
        }
    }

    const float rangeLuma = maxLuma - minLuma;
    if(rangeLuma < max(EDGE_THRESHOLD_MIN, EDGE_THRESHOLD_MAX * maxLuma))
    {
        outFragColor = vec4(rgb[Center], 1.0);
        return;
    }

    const float lumaTopBottom = lumas[Top] + lumas[Bottom];
    const float lumaLeftRight = lumas[Left] + lumas[Right];
    const float lumaTopCorners = lumas[TopLeft] + lumas[TopRight];
    const float lumaBottomCorners = lumas[BottomLeft] + lumas[BottomRight];
    const float lumaLeftCorners = lumas[TopLeft] + lumas[BottomLeft];
    const float lumaRightCorners = lumas[TopRight] + lumas[BottomRight];
    const float lumaTBLR = lumaTopBottom + lumaLeftRight;

    const float averageLumaTBLR = (lumaTBLR) / 4.0;
    const float lumaSubRange = abs(averageLumaTBLR - lumas[Center]);
    float pixelblendAmount = max(0.0, (lumaSubRange / rangeLuma) - MIN_PIXEL_ALIASING_REQUIRED);
    pixelblendAmount = min(PIXEL_BLEND_LIMIT_TO_REDUCE_BLURRING, pixelblendAmount * (1.0 / (1.0 - MIN_PIXEL_ALIASING_REQUIRED)));

    const vec3 averageRGBNeighbor = rgbSum * (1.0 / 9.0);
    const float verticalEdgeRow1 = abs(-2.0 * lumas[Top] + lumaTopCorners);
    const float verticalEdgeRow2 = abs(-2.0 * lumas[Center] + lumaLeftRight);
    const float verticalEdgeRow3 = abs(-2.0 * lumas[Bottom] + lumaBottomCorners);
    const float verticalEdge = (verticalEdgeRow1 + verticalEdgeRow2 * 2.0 + verticalEdgeRow3) / 12.0;
    const float horizontalEdgeCol1 = abs(-2.0 * lumas[Left] + lumaLeftCorners);
    const float horizontalEdgeCol2 = abs(-2.0 * lumas[Center] + lumaTopBottom);
    const float horizontalEdgeCol3 = abs(-2.0 * lumas[Right] + lumaRightCorners);
    const float horizontalEdge = (horizontalEdgeCol1 + horizontalEdgeCol2 * 2.0 + horizontalEdgeCol3) / 12.0;
    const bool isHorizontal = horizontalEdge >= verticalEdge;
    const float luma1 =isHorizontal ? lumas[Top] : lumas[Left];
    const float luma2 = isHorizontal ? lumas[Bottom] : lumas[Right];
    const bool is1Steepest = abs(lumas[Center] - luma1) >= abs(lumas[Center] - luma2);
    float stepLength = isHorizontal ? -viewportSizeInverse.y : -viewportSizeInverse.x;

    float lumaHighContrastPixel;
    if (is1Steepest) 
    {
        lumaHighContrastPixel = luma1;
    }
    else
    {
        lumaHighContrastPixel = luma2;
        stepLength = -stepLength;
    }

    vec2 outPosToFetchTexelForEdgeAntiAliasing;
    vec3 rgbEdgeAntiAliasingPixel = rgb[Center];

    const float res = findEndPointPosition(inUV, lumas[Center], lumaHighContrastPixel, stepLength,
            viewportSizeInverse, isHorizontal, outPosToFetchTexelForEdgeAntiAliasing);
    
    if(res == 1.0)
    {
        rgbEdgeAntiAliasingPixel = texture(globalTextures[nonuniformEXT(PushConstants.drawImageIndex)], outPosToFetchTexelForEdgeAntiAliasing).rgb;
    }

    vec4 finalColor = vec4(mix(rgbEdgeAntiAliasingPixel, averageRGBNeighbor, pixelblendAmount), 1.0);
    outFragColor = finalColor;
}

float findEndPointPosition(vec2 textureCoordMiddle, 
                            float lumaMiddle,
                            float lumaHighContrastPixel,
                            float stepLength,
                            vec2 viewportSizeInverse,
                            bool isHorizontal,
                            out vec2 outPosToFetchTexelForEdgeAntiAliasing)
{
    vec2 textureCoordOfHighContrastPixel = textureCoordMiddle;
    vec2 edgeDir;

    if (isHorizontal) {
        textureCoordOfHighContrastPixel.y = textureCoordMiddle.y + stepLength;
        textureCoordOfHighContrastPixel.x = textureCoordMiddle.x;
        edgeDir.x = viewportSizeInverse.x;
        edgeDir.y = 0.0;
    } else {
        textureCoordOfHighContrastPixel.x = textureCoordMiddle.x + stepLength;
        textureCoordOfHighContrastPixel.y = textureCoordMiddle.y;
        edgeDir.y = viewportSizeInverse.y;
        edgeDir.x = 0.0;
    }

    float lumaHighContrastPixelNegDir;
    float lumaHighContrastPixelPosDir;
    float lumaMiddlePixelNegDir;
    float lumaMiddlePixelPosDir;
    bool doneGoingThroughNegDir = false;
    bool doneGoingThroughPosDir = false;
    vec2 posHighContrastNegDir = textureCoordOfHighContrastPixel - edgeDir;
    vec2 posHighContrastPosDir = textureCoordOfHighContrastPixel + edgeDir;
    vec2 posMiddleNegDir = textureCoordMiddle - edgeDir;
    vec2 posMiddlePosDir = textureCoordMiddle + edgeDir;
    for (int i = 0; i < NUM_LOOP_FOR_EDGE_DETECTION; ++i) {
        if (!doneGoingThroughNegDir) {
        lumaHighContrastPixelNegDir =
            rgb2luma(texture(globalTextures[nonuniformEXT(PushConstants.drawImageIndex)], posHighContrastNegDir).rgb);
        lumaMiddlePixelNegDir =
            rgb2luma(texture(globalTextures[nonuniformEXT(PushConstants.drawImageIndex)], posMiddleNegDir).rgb);
        doneGoingThroughNegDir =
            abs(lumaHighContrastPixelNegDir - lumaHighContrastPixel) >
                abs(lumaHighContrastPixelNegDir - lumaMiddle) ||
            abs(lumaMiddlePixelNegDir - lumaMiddle) >
                abs(lumaMiddlePixelNegDir - lumaHighContrastPixel);
        }
        if (!doneGoingThroughPosDir) {
        lumaHighContrastPixelPosDir =
            rgb2luma(texture(globalTextures[nonuniformEXT(PushConstants.drawImageIndex)], posHighContrastPosDir).rgb);
        lumaMiddlePixelPosDir =
            rgb2luma(texture(globalTextures[nonuniformEXT(PushConstants.drawImageIndex)], posMiddlePosDir).rgb);
        doneGoingThroughPosDir =
            abs(lumaHighContrastPixelPosDir - lumaHighContrastPixel) >
                abs(lumaHighContrastPixelPosDir - lumaMiddle) ||
            abs(lumaMiddlePixelPosDir - lumaMiddle) >
                abs(lumaMiddlePixelPosDir - lumaHighContrastPixel);
        }
        if (doneGoingThroughNegDir && doneGoingThroughPosDir) {
        break;
        }
        if (!doneGoingThroughNegDir) {
        posHighContrastNegDir -= edgeDir;
        posMiddleNegDir -= edgeDir;
        }
        if (!doneGoingThroughPosDir) {
        posHighContrastPosDir += edgeDir;
        posMiddlePosDir += edgeDir;
        }
    }
    float dstNeg;
    float dstPos;
    if (isHorizontal) {
        dstNeg = textureCoordMiddle.x - posMiddleNegDir.x;
        dstPos = posMiddlePosDir.x - textureCoordMiddle.x;
    } else {
        dstNeg = textureCoordMiddle.y - posMiddleNegDir.y;
        dstPos = posMiddlePosDir.y - textureCoordMiddle.y;
    }
    bool isMiddlePixelCloserToNeg = dstNeg < dstPos;
    float dst = min(dstNeg, dstPos);
    float lumaEndPointOfPixelCloserToMiddle =
        isMiddlePixelCloserToNeg ? lumaMiddlePixelNegDir : lumaMiddlePixelPosDir;
    bool edgeAARequired =
        abs(lumaEndPointOfPixelCloserToMiddle - lumaHighContrastPixel) <
        abs(lumaEndPointOfPixelCloserToMiddle - lumaMiddle);
    // Compute the pixel offset:
    float negInverseEndPointsLength = -1.0 / (dstNeg + dstPos);
    float pixelOffset = dst * negInverseEndPointsLength + 0.5;
    outPosToFetchTexelForEdgeAntiAliasing = textureCoordMiddle;
    if (isHorizontal) {
        outPosToFetchTexelForEdgeAntiAliasing.y += pixelOffset * stepLength;
    } else {
        outPosToFetchTexelForEdgeAntiAliasing.x += pixelOffset * stepLength;
    }
    return edgeAARequired ? 1.0 : 0.0;
}