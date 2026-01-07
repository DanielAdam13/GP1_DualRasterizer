#include "Matrix.h"
#include "Texture.h"

class MeshBase
{
public:
	virtual ~MeshBase() = default;

	virtual Matrix GetWorldMatrix() const = 0;
	virtual const std::vector<VertexIn>& GetVertices() const = 0;
	virtual const std::vector<uint32_t>& GetIndices() const = 0;

	virtual const Texture* GetDiffuseTexture() const = 0;
	virtual const Texture* GetNormalTexture() const = 0;
	virtual const Texture* GetSpecularTexture() const = 0;
	virtual const Texture* GetGlossTexture() const = 0;
private:

};