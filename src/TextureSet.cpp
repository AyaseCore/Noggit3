#include "TextureSet.h"

#include "Brush.h"
#include "Environment.h"
#include "TextureManager.h" // TextureManager, Texture
#include "Video.h"
#include "MapHeaders.h"
#include "MapTile.h"
#include "Log.h"
#include "World.h"

#include <iostream>     // std::cout
#include <algorithm>    // std::min

TextureSet::TextureSet()
{}

TextureSet::~TextureSet()
{
	for (size_t i = 1; i < nTextures; ++i)
		delete alphamaps[i - 1];
}

void TextureSet::initTextures(MPQFile* f, MapTile* maintile, uint32_t size)
{
	// texture info
	nTextures = size / 16U;

	for (size_t i = 0; i<nTextures; ++i) {
		f->read(&tex[i], 4);
		f->read(&texFlags[i], 4);
		f->read(&MCALoffset[i], 4);
		f->read(&effectID[i], 4);

		if (texFlags[i] & FLAG_ANIMATE)
		{
			animated[i] = texFlags[i];
		}
		else
		{
			animated[i] = 0;
		}
		textures[i] = TextureManager::newTexture(maintile->mTextureFilenames[tex[i]]);
	}
}

void TextureSet::initAlphamaps(MPQFile* f, size_t nLayers, bool mBigAlpha, bool doNotFixAlpha)
{
	unsigned int MCALbase = f->getPos();

	for (size_t i = 0; i < 3; ++i)
	{
		alphamaps[i] = NULL;
	}

	for (unsigned int layer = 0; layer < nLayers; ++layer)
	{
		if (texFlags[layer] & 0x100)
		{
			f->seek(MCALbase + MCALoffset[layer]);
			alphamaps[layer - 1] = new Alphamap(f, texFlags[layer], mBigAlpha, doNotFixAlpha);
		}
	}

  // convert big alphas to the old format to be rendered correctly in noggit
  if (mBigAlpha)
  {
    convertToOldAlpha();
  }
}

int TextureSet::addTexture(OpenGL::Texture* texture)
{
	int texLevel = -1;

	if (nTextures < 4U)
	{
		texLevel = nTextures;
		nTextures++;

    texture->addReference();

		textures[texLevel] = texture;    
		animated[texLevel] = 0;
		texFlags[texLevel] = 0;
		effectID[texLevel] = 0;

		if (texLevel)
		{
			if (alphamaps[texLevel - 1])
			{
				LogError << "Alpha Map has invalid texture binding" << std::endl;
				nTextures--;
				return -1;
			}
			alphamaps[texLevel - 1] = new Alphamap();
		}
	}

	return texLevel;
}

void TextureSet::switchTexture(OpenGL::Texture* oldTexture, OpenGL::Texture* newTexture)
{
	int texLevel = -1;
	for (size_t i = 0; i < nTextures; ++i)
	{
		if (textures[i] == oldTexture)
			texLevel = i;
		// prevent texture duplication
		if (textures[i] == newTexture) 
			return;
	}		

	if (texLevel != -1)
	{
		textures[texLevel] = newTexture;
	}
}

// swap 2 textures of a chunk with their alpha
void TextureSet::swapTexture(int id1, int id2)
{
  if (id1 >= 0 && id2 >= 0 && id1 < nTextures && id2 < nTextures)
  {
    OpenGL::Texture* temp = textures[id1];
    textures[id1] = textures[id2];
    textures[id2] = temp;

    for (int j = 0; j < 64; j++)
    {
      for (int i = 0; i < 64; ++i)
      {
        float alphas[3] = { 0.0f, 0.0f, 0.0f };
        float visibility[4] = { 255.0f, 0.0f, 0.0f, 0.0f };

        for (size_t k = 0; k < nTextures - 1; k++)
        {
          float f = static_cast<float>(alphamaps[k]->getAlpha(i + j * 64));
          visibility[k + 1] = f;
          alphas[k] = f;
          for (size_t n = 0; n <= k; n++)
            visibility[n] = (visibility[n] * ((255.0f - f)) / 255.0f);
        }

        float tmp = visibility[id1];
        visibility[id1] = visibility[id2];
        visibility[id2] = tmp;

        for (int k = nTextures - 2; k >= 0; k--)
        {
          alphas[k] = visibility[k + 1];
          for (int n = nTextures - 2; n > k; n--)
          {
            // prevent 0 division
            if (alphas[n] == 255.0f)
            {
              alphas[k] = 0.0f;
              break;
            }
            else
              alphas[k] = (alphas[k] / (255.0f - alphas[n])) * 255.0f;
          }
        }

        for (size_t k = 0; k < nTextures - 1; k++)
        {
          alphamaps[k]->setAlpha(i + j * 64, static_cast<unsigned char>(std::min(std::max(alphas[k], 0.0f), 255.0f)));
          alphamaps[k]->loadTexture();
        }
      }
    }
  }
}

void TextureSet::eraseTextures()
{
	for (size_t i = nTextures-1; nTextures; --i)
	{
    eraseTexture(i);
	}
}

void TextureSet::eraseTexture(size_t id)
{
  if (id > 3)
    return;

  TextureManager::delbyname(textures[id]->filename());
  tex[id] = 0;

  if (id)
  {
    delete alphamaps[id - 1];
    alphamaps[id - 1] = nullptr;
  }

  // shift textures above
  for (size_t i = id; i < nTextures - 1; i++)
  {
    if (i)
    {
      alphamaps[i - 1] = alphamaps[i];
    }

    tex[i] = tex[i + 1];
    textures[i] = textures[i + 1];
    animated[i] = animated[i + 1];
    texFlags[i] = texFlags[i + 1];
    effectID[i] = effectID[i + 1];
  }

  alphamaps[nTextures - 2] = nullptr;
  tex[nTextures - 1] = 0;

  nTextures--;
}

const std::string& TextureSet::filename(size_t id)
{
	return textures[id]->filename();
}

void TextureSet::bindAlphamap(size_t id, size_t activeTexture)
{
	OpenGL::Texture::enableTexture(activeTexture);

	alphamaps[id]->bind();
}

void TextureSet::bindTexture(size_t id, size_t activeTexture)
{
	OpenGL::Texture::enableTexture(activeTexture);

	textures[id]->bind();
}

void TextureSet::start2DAnim(int id)
{
	if (id < 0)
		return;

	if (animated[id])
	{
		OpenGL::Texture::setActiveTexture(0);
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();

		// note: this is ad hoc and probably completely wrong
		const int spd = (animated[id] & 0x08) | ((animated[id] & 0x10) >> 2) | ((animated[id] & 0x20) >> 4) | ((animated[id] & 0x40) >> 6);
		const int dir = animated[id] & 0x07;
		const float texanimxtab[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
		const float texanimytab[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
		const float fdx = -texanimxtab[dir], fdy = texanimytab[dir];

		const float f = (static_cast<int>(gWorld->animtime * (spd / 15.0f)) % 1600) / 1600.0f;
		glTranslatef(f*fdx, f*fdy, 0);
	}
}

void TextureSet::stop2DAnim(int id)
{
	if (id < 0)
		return;

	if (animated[id])
	{
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		OpenGL::Texture::setActiveTexture(1);
	}
}

//! \todo do they really differ? investigate
void TextureSet::startAnim(int id)
{
	if (id < 0)
		return;

	if (animated[id])
	{
		OpenGL::Texture::setActiveTexture(0);
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();

		// note: this is ad hoc and probably completely wrong
		const int spd = (animated[id] & 0x08) | ((animated[id] & 0x10) >> 2) | ((animated[id] & 0x20) >> 4) | ((animated[id] & 0x40) >> 6);
		const int dir = animated[id] & 0x07;
		const float texanimxtab[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
		const float texanimytab[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
		const float fdx = -texanimxtab[dir], fdy = texanimytab[dir];
		const int animspd = (const int)(200 * detail_size);
		float f = ((static_cast<int>(gWorld->animtime*(spd / 15.0f))) % animspd) / static_cast<float>(animspd);
		glTranslatef(f*fdx, f*fdy, 0);
	}
}

void TextureSet::stopAnim(int id)
{
	if (id < 0)
		return;

	if (animated[id])
	{
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		OpenGL::Texture::setActiveTexture(1);
	}
}

bool TextureSet::eraseUnusedTextures()
{
  bool texRemoved = false;

  if (nTextures < 2)
    return texRemoved;
    
  unsigned char alpha[64 * 64];
  bool baseVisible = false;
  size_t texCount = nTextures;

  for (size_t k = nTextures - 1; k > 0; k--)
  {
    bool texVisible = false;
    // use a temp variable because nTexture can be decreased at the end of the loop
    // if the texture above is fully opaque, don't test textures bellow
    if (k >= texCount - 1 || baseVisible)
    {
      // reset baseVisible because this layer could be fully opaque
      baseVisible = false;
      memcpy(alpha, alphamaps[k - 1]->getAlpha(), 64 * 64);
      for (size_t i = 0; i < 64 * 64; i++)
      {
        unsigned char a = alpha[i];
        if (a > 0)
        {
          texVisible = true;

          if (a < 255)
          {
            baseVisible = true;
            break;
          }
        }
        else
        {
          baseVisible = true;
        }
      }
    }

    if (!texVisible)
    {
      eraseTexture(k);
      texRemoved = true;
    }
  }
  
  // there will always be at least 2 textures when entering the condition
  if (!baseVisible)
  {
    // swap the base layer with the layer above
    swapTexture(0, 1);
    eraseTexture(1);
    texRemoved = true;
  }

  return texRemoved;
}

bool TextureSet::paintTexture(float xbase, float zbase, float x, float z, Brush* brush, float strength, float pressure, OpenGL::Texture* texture)
{
  bool changed = false;

	if (Environment::getInstance()->paintMode == true)
	{
		float zPos, xPos, xdiff, zdiff, dist, radius;  

		//xbase, zbase mapchunk pos
		//x, y mouse pos

		int texLevel = -1;

		radius = brush->getRadius();

		xdiff = xbase - x + CHUNKSIZE / 2;
		zdiff = zbase - z + CHUNKSIZE / 2;
		dist = sqrt(xdiff*xdiff + zdiff*zdiff);

		if (dist > (radius + MAPCHUNK_RADIUS))
			return changed;

		//First Lets find out do we have the texture already
		for (size_t i = 0; i<nTextures; ++i)
			if (textures[i] == texture)
				texLevel = i;

    if (texLevel == -1 && strength == 0)
    {
      return false;
    }

		if ((texLevel == -1) && (nTextures == 4) && !eraseUnusedTextures())
		{
			LogDebug << "paintTexture: No free texture slot" << std::endl;
			return false;
		}

		//Only 1 layer and its that layer
		if ((texLevel != -1) && (nTextures == 1))
			return true;

		if (texLevel == -1)
		{
			texLevel = addTexture(texture);
			if (texLevel == 0)
				return true;
			if (texLevel == -1)
			{
				LogDebug << "paintTexture: Unable to add texture." << std::endl;
				return false;
			}
		}

		zPos = zbase;
    bool texVisible[4] = { false, false, false, false };

		for (int j = 0; j < 64; j++)
		{
			xPos = xbase;
			for (int i = 0; i < 64; ++i)
			{
				xdiff = xPos - x + (TEXDETAILSIZE / 2.0f); // Use the center instead of
				zdiff = zPos - z + (TEXDETAILSIZE / 2.0f); // the top left corner
				dist = std::abs(std::sqrt(xdiff*xdiff + zdiff*zdiff));

				if (dist>radius)
				{
          bool baseVisible = true;
          for (size_t k = nTextures - 1; k > 0; k--)
          {
            unsigned char a = alphamaps[k - 1]->getAlpha(i + j * 64);

            if (a > 0)
            {
              texVisible[k] = true;
              
              if (a == 255)
              {
                baseVisible = false;
              }
            }
          }
          texVisible[0] = texVisible[0] || baseVisible;

					xPos += TEXDETAILSIZE;
					continue;
				}				

        float tPressure = pressure*brush->getValue(dist);
        float alphas[3] = { 0.0f, 0.0f, 0.0f };
        float visibility[4] = { 255.0f, 0.0f, 0.0f, 0.0f };
        
        for (size_t k = 0; k < nTextures - 1; k++)
        {
          float f = static_cast<float>(alphamaps[k]->getAlpha(i + j * 64));
          visibility[k+1] = f;
          alphas[k] = f;
          for (size_t n = 0; n <= k; n++)
            visibility[n] = (visibility[n] * ((255.0f - f)) / 255.0f);
        }

        // nothing to do
        if (visibility[texLevel] == strength)
        {
          for (size_t k = 0; k < nTextures; k++)
          {
            texVisible[k] = texVisible[k] || (visibility[k] > 0.0f);
          }

          xPos += TEXDETAILSIZE;
          continue;
        }

        // at this point we know for sure that the textures will be changed
        changed = true;

        // alpha delta
        float diffA = (strength - visibility[texLevel])* tPressure;

        // visibility = 255 => all other at 0
        if (visibility[texLevel] + diffA >= 255.0f)
        {
          for (size_t k = 0; k < nTextures; k++)
          {
            visibility[k] = (k == texLevel) ? 255.0f : 0.0f;
          }
        }
        else
        {
          float other = 255.0f - visibility[texLevel];

          if (visibility[texLevel] == 255.0f && diffA < 0.0f)
          {
            visibility[texLevel] += diffA;
            int idTex = (!texLevel) ? 1 : texLevel - 1; // nTexture > 1 else it'd have returned true at the beginning
            visibility[idTex] -= diffA; 
          }
          else
          {
            visibility[texLevel] += diffA;

            for (size_t k = 0; k < nTextures; k++)
            {
              if (k == texLevel || visibility[k] == 0)
                continue;

              visibility[k] = visibility[k] - (diffA * (visibility[k] / other));
            }
          }          
        }

        for (int k = nTextures - 2; k >= 0; k--)
        {
          alphas[k] = visibility[k+1];
          for (int n = nTextures - 2; n > k; n--)
          {
            // prevent 0 division
            if (alphas[n] == 255.0f)
            {
              alphas[k] = 0.0f;
              break;
            }
            else
              alphas[k] = (alphas[k] / (255.0f - alphas[n])) * 255.0f;
          }
        }

        for (size_t k = 0; k < nTextures; k++)
        {
          if (k < nTextures - 1)
          {
            alphamaps[k]->setAlpha(i + j * 64, static_cast<unsigned char>(std::min(std::max(round(alphas[k]), 0.0f), 255.0f)));
          }
          texVisible[k] = texVisible[k] || (visibility[k] > 0.0f);
        }

				xPos += TEXDETAILSIZE;
			}
			zPos += TEXDETAILSIZE;
		}

    if (!changed)
    {
      return false;
    }

    // stop after k=0 because k is unsigned
    for (size_t k = nTextures - 1; k < 4; k--)
    {
      if (!texVisible[k])
        eraseTexture(k);
    }

    if (nTextures < 2)
    {
      return changed;
    }

		for (size_t j = 0; j < nTextures - 1; j++)
		{
			if (j > 2)
			{
				LogError << "WTF how did you get here??? Get a cookie." << std::endl;
				continue;
			}

			alphamaps[j]->loadTexture();
		}
	}

	return changed;
}

const size_t TextureSet::num()
{
	return nTextures;
}

const unsigned int TextureSet::flag(size_t id)
{
	return texFlags[id];
}

const unsigned int TextureSet::effect(size_t id)
{
	return effectID[id];
}

void TextureSet::setAlpha(size_t id, size_t offset, unsigned char value)
{
	alphamaps[id]->setAlpha(offset, value);
}

void TextureSet::setAlpha(size_t id, unsigned char *amap)
{
	alphamaps[id]->setAlpha(amap);
}

const unsigned char TextureSet::getAlpha(size_t id, size_t offset)
{
	return alphamaps[id]->getAlpha(offset);
}

const unsigned char *TextureSet::getAlpha(size_t id)
{
	return alphamaps[id]->getAlpha();
}

OpenGL::Texture* TextureSet::texture(size_t id)
{
	return textures[id];
}


void TextureSet::convertToBigAlpha()
{
  // nothing to do
  if (nTextures < 2)
    return;

  unsigned char tab[3][64 * 64];

  for (size_t k = 0; k < nTextures - 1; k++)
  {
    memcpy(tab[k], alphamaps[k]->getAlpha(), 64 * 64);
  }

  float alphas[3] = { 0.0f, 0.0f, 0.0f };

  for (int i = 0; i < 64 * 64; ++i)
  {
    for (size_t k = 0; k < nTextures - 1; k++)
    {
      float f = static_cast<float>(tab[k][i]);
      alphas[k] = f;
      for (size_t n = 0; n < k; n++)
        alphas[n] = (alphas[n] * ((255.0f - f)) / 255.0f);
    }

    for (size_t k = 0; k < nTextures - 1; k++)
    {
      tab[k][i] = static_cast<unsigned char>(std::min(std::max(round(alphas[k]), 0.0f), 255.0f));
    }
  }

  for (size_t k = 0; k < nTextures - 1; k++)
  {
    alphamaps[k]->setAlpha(tab[k]);
    alphamaps[k]->loadTexture();
  }
}

void TextureSet::convertToOldAlpha()
{
  // nothing to do
  if (nTextures < 2)
    return;

  unsigned char tab[3][64 * 64];

  for (size_t k = 0; k < nTextures - 1; k++)
  {
    memcpy(tab[k], alphamaps[k]->getAlpha(), 64 * 64);
  }    

  float alphas[3] = { 0.0f, 0.0f, 0.0f };

  for (int i = 0; i < 64 * 64; ++i)
  {
    for (size_t k = 0; k < nTextures - 1; k++)
    {
      alphas[k] = static_cast<float>(tab[k][i]);
    }

    for (int k = nTextures - 2; k >= 0; k--)
    {
      for (int n = nTextures - 2; n > k; n--)
      {
        // prevent 0 division
        if (alphas[n] == 255.0f)
        {
          alphas[k] = 0.0f;
          break;
        }
        else
          alphas[k] = (alphas[k] / (255.0f - alphas[n])) * 255.0f;
      }
    }

    for (size_t k = 0; k < nTextures - 1; k++)
    {
      tab[k][i] = static_cast<unsigned char>(std::min(std::max(round(alphas[k]), 0.0f), 255.0f));
    }
  }
  
  for (size_t k = 0; k < nTextures - 1; k++)
  {
    alphamaps[k]->setAlpha(tab[k]);
    alphamaps[k]->loadTexture();
  }
}

void TextureSet::mergeAlpha(size_t id1, size_t id2)
{
  if (id1 >= nTextures || id2 >= nTextures || id1 == id2)
    return;

  if (!id1)
  {
    eraseTexture(id2);
    return;
  }
  if (!id2)
  {
    eraseTexture(id1);
    return;
  }

  unsigned char tab[3][64 * 64];

  for (size_t k = 0; k < nTextures - 1; k++)
  {
    memcpy(tab[k], alphamaps[k]->getAlpha(), 64 * 64);
  }

  float alphas[3] = { 0.0f, 0.0f, 0.0f };
  float visibility[4] = { 255.0f, 0.0f, 0.0f, 0.0f };

  for (int i = 0; i < 64 * 64; ++i)
  {
    for (size_t k = 0; k < nTextures - 1; k++)
    {
      float f = static_cast<float>(tab[k][i]);
      alphas[k] = f;
      for (size_t n = 0; n < k; n++)
        alphas[n] = (alphas[n] * ((255.0f - f)) / 255.0f);
    }

    for (size_t k = 0; k < nTextures - 1; k++)
    {
      float f = static_cast<float>(tab[k][i]);
      visibility[k + 1] = f;
      for (size_t n = 0; n <= k; n++)
        visibility[n] = (visibility[n] * ((255.0f - f)) / 255.0f);
    }

    visibility[id1] += visibility[id2];
    visibility[id2] = 0;

    for (int k = nTextures - 2; k >= 0; k--)
    {
      alphas[k] = visibility[k + 1];
      for (int n = nTextures - 2; n > k; n--)
      {
        // prevent 0 division
        if (alphas[n] == 255.0f)
        {
          alphas[k] = 0.0f;
          break;
        }
        else
          alphas[k] = (alphas[k] / (255.0f - alphas[n])) * 255.0f;
      }
    }

    for (size_t k = 0; k < nTextures - 1; k++)
    {
      tab[k][i] = static_cast<unsigned char>(std::min(std::max(round(alphas[k]), 0.0f), 255.0f));
    }
  }


  eraseTexture(id2);

  for (size_t k = 0; k < nTextures - 1; k++)
  {
    alphamaps[k]->setAlpha(tab[k]);
    alphamaps[k]->loadTexture();
  }
}

bool TextureSet::removeDuplicate()
{
  bool changed = false;

  for (size_t i = 0; i < nTextures; i++)
  {
    for (size_t j = i + 1; j < nTextures; j++)
    {
      if (textures[i] == textures[j])
      {
        mergeAlpha(i, j);
        changed = true;
      }
    }
  }

  return changed;
}
