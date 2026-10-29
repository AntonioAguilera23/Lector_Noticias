
#pragma once

#include <string>
#include <vector>

#include "agro/PricePoint.h"

namespace agro {

class PriceSource {
public:
    virtual ~PriceSource() = default;

    virtual std::vector<PricePoint> load(
        const std::string& path
    ) = 0;

    virtual const std::vector<std::string>& warnings() const = 0;
};

} // namespace agro
