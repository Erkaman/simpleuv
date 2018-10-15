#include <simpleuv/chartpacker.h>
#include <cmath>
extern "C" {
#include <maxrects.h>
}
#include <QDebug>

namespace simpleuv
{

void ChartPacker::setCharts(const std::vector<std::pair<float, float>> &chartSizes)
{
    m_chartSizes = chartSizes;
}

const std::vector<std::tuple<float, float, float, float, bool>> &ChartPacker::getResult()
{
    return m_result;
}

double ChartPacker::calculateTotalArea()
{
    double totalArea = 0;
    for (const auto &chartSize: m_chartSizes) {
        totalArea += chartSize.first * chartSize.second;
    }
    return totalArea;
}

bool ChartPacker::tryPack(float textureSize)
{
    std::vector<maxRectsSize> rects;
    int width = textureSize * m_floatToIntFactor;
    int height = width;
    qDebug() << "Try the " << m_tryNum << "nth times pack with factor:" << m_textureSizeFactor << " size:" << width << "x" << height;
    for (const auto &chartSize: m_chartSizes) {
        maxRectsSize r;
        r.width = chartSize.first * m_floatToIntFactor;
        r.height = chartSize.second * m_floatToIntFactor;
        qDebug() << "  :chart " << r.width << "x" << r.height;
        rects.push_back(r);
    }
    const maxRectsFreeRectChoiceHeuristic methods[] = {
        rectBestShortSideFit,
        rectBestLongSideFit,
        rectBestAreaFit,
        rectBottomLeftRule,
        rectContactPointRule
    };
    float occupancy = 0;
    float bestOccupancy = 0;
    std::vector<maxRectsPosition> bestResult;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i) {
        qDebug() << "Test method[" << methods[i] << "]";
        std::vector<maxRectsPosition> result(rects.size());
        if (0 != maxRects(width, height, rects.size(), rects.data(), methods[i], true, result.data(), &occupancy)) {
            qDebug() << "  method[" << methods[i] << "] failed";
            continue;
        }
        qDebug() << "  method[" << methods[i] << "] occupancy:" << occupancy;
        if (occupancy > bestOccupancy) {
            bestResult = result;
            bestOccupancy = occupancy;
        }
    }
    if (bestResult.size() != rects.size())
        return false;
    m_result.resize(bestResult.size());
    for (decltype(bestResult.size()) i = 0; i < bestResult.size(); ++i) {
        const auto &result = bestResult[i];
        const auto &rect = rects[i];
        auto &dest = m_result[i];
        dest = {(float)result.left / width, (float)result.top / height, (float)rect.width / width, (float)rect.height / height, result.rotated};
        //qDebug() << "result[" << i << "]:" << std::get<0>(dest) << std::get<1>(dest) << std::get<2>(dest) << std::get<3>(dest) << std::get<4>(dest);
    }
    return true;
}

void ChartPacker::pack()
{
    float initialGuessSize = std::sqrt(calculateTotalArea() * m_initialAreaGuessFactor);
    while (true) {
        float textureSize = initialGuessSize * m_textureSizeFactor;
        ++m_tryNum;
        if (tryPack(textureSize))
            break;
        m_textureSizeFactor += m_textureSizeGrowFactor;
    }
}

}
