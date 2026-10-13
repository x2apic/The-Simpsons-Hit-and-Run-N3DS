#ifndef _HCTUNESHADER_HPP_
#define _HCTUNESHADER_HPP_

#include"../pddi.hpp"
#include"shader.hpp"

class d3dContext;
class d3dTexture;

class d3dHKToonShader: public d3dShader
{
public:
   d3dHKToonShader(d3dContext* c);
   ~d3dHKToonShader();

   const char* GetType(void);
   int  GetPasses();

protected:
   static pddiShadeColourTable colourTable[];
   static pddiShadeTextureTable textureTable[];
   static pddiShadeIntTable intTable[];
   static pddiShadeFloatTable floatTable[];

   void PreRender(void);
   void PostRender(void);

   void SetPass(int pass = 0);

   pddiShadeTextureTable* GetTextureTable(void) { return textureTable;}
   pddiShadeIntTable*     GetIntTable(void)     { return intTable;}
   pddiShadeFloatTable*   GetFloatTable(void)   { return floatTable;}
   pddiShadeColourTable*  GetColourTable(void)  { return colourTable;}

   
   //Pixel shader
   unsigned long m_PixelShaderHandle;


   void SetTexture(pddiTexture* t);
   void SetTuneMap(pddiTexture* t);

   // texturing
   d3dTexture* m_pTexture;
   d3dTexture* m_pTuneMap;
};

#endif //_HCTUNESHADER_HPP_