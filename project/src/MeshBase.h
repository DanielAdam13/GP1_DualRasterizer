#include "Matrix.h"

class MeshBase
{
public:
	virtual ~MeshBase() = default;

	virtual Matrix GetWorldMatrix() const = 0;
private:

};