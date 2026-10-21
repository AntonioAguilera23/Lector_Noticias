#pragma once
#include "agro/PricePoint.h"
#include <string>
#include <vector>
using namespace std;

namespace agro{

class PriceSource {
    
    public:
        // 1. Destructor virtual: OBLIGATORIO en cualquier clase base polimórfica.
        virtual ~PriceSource() = default;

        // 2. Método virtual puro (= 0): obliga a las clases hijas a implementarlo.
        virtual vector<PricePoint> load(const string& path) = 0;

        // 3. Contrato para consultar advertencias no fatales durante la carga.
        virtual const vector<string>& warnings() const = 0;
    };

} 