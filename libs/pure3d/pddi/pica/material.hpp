#ifndef PDDI_PICA_MATERIAL_HPP
#define PDDI_PICA_MATERIAL_HPP

#include <pddi/pica/pica.hpp>
#include <pddi/base/baseshader.hpp>

const int picaMaxPasses = 1;

struct picaTextureEnv
{
    bool enabled;
    picaTexture* texture;

    int uvSet;
    pddiTextureGen texGen;
    pddiUVMode uvMode;
    pddiFilterMode filterMode;

    bool alphaTest;
    pddiBlendMode alphaBlendMode;
    pddiCompareMode alphaCompareMode;
    float alphaRef;
    int writeMask;

    bool lit;
    bool twoSided;
    pddiShadeMode shadeMode;
    pddiColour diffuse;
    pddiColour specular;
    pddiColour ambient;
    pddiColour emissive;
    float shininess;
};

class picaMat : public pddiBaseShader
{
public:
    picaMat(picaContext*);
    ~picaMat();

    static pddiShadeColourTable colourTable[];
    static pddiShadeTextureTable textureTable[];
    static pddiShadeIntTable intTable[];
    static pddiShadeFloatTable floatTable[];

    const char* GetType(void);
    int         GetPasses(void);
    void        SetPass(int pass);

    pddiShadeTextureTable* GetTextureTable(void) { return textureTable; }
    pddiShadeIntTable*     GetIntTable(void)     { return intTable; }
    pddiShadeFloatTable*   GetFloatTable(void)   { return floatTable; }
    pddiShadeColourTable*  GetColourTable(void)  { return colourTable; }

    void SetTexture(pddiTexture* texture);
    void SetUVMode(int mode);
    void SetFilterMode(int mode);

    void SetShadeMode(int shade);
    void SetTwoSided(int);

    void EnableLighting(int);

    void SetDiffuse(pddiColour colour);
    void SetAmbient(pddiColour colour);
    void SetEmissive(pddiColour);
    void SetEmissiveAlpha(int);
    void SetSpecular(pddiColour);
    void SetShininess(float power);

    void SetBlendMode(int mode);
    void EnableAlphaTest(int);
    void SetAlphaCompare(int compare);
    void SetAlphaRef(float ref);

    void SetColourWrite(int mask);

    int  CountDevPasses(void);
    void SetDevPass(unsigned);
    void PreRender(void);

private:
    picaContext* context;
    int pass;
    bool dirty;
    picaTextureEnv texEnv[picaMaxPasses];
};
#endif
