#ifndef ENVELOPESINTERPOLATOR_H
#define ENVELOPESINTERPOLATOR_H

#include <vector>
#include <algorithm>
#include <cmath>

/*
    This class performs interpolation between a set of shapes, each defined by a set of points.
    Although primarily intended for audio envelope interpolation, it can be used for other purposes.

    Key considerations for interpolation:
    1. Preserving the peak of the envelope.
    2. Avoiding involuntary discontinuities.

    The interpolation is "circle-shaped," meaning any shape can be interpolated with the next and 
    previous shapes in the sequence, with the last shape connecting seamlessly to the first.
    Because the primary purpose of this class is for audio envelopes, it is assumed that
    the first and last points of each shape are zero.
*/

struct EnvelopeTable {
        const float* data;
        int envsize;
        int numberOfShapes;
        std::vector<int> peaks;
};

class EnvelopesInterpolator
{
public:
    EnvelopesInterpolator(int envsize);
    EnvelopesInterpolator(EnvelopeTable e);

    /**
      * @brief Interpolates between two shapes based on a given factor.
      * 
      * @param s Interpolation factor (0.0 ≤ s ≤ _numberOfShapes).
      * @param targetbuffer Target buffer to store the interpolated shape.
      */
    void interpolate(float s, float* targetbuffer);
    void interpolate(float s, std::vector<float>& targetbuffer);

    void setDataAndPeaks(const float* data, const std::vector<int>& peaks);
    void setEnvelopeTable(EnvelopeTable e);
    void addNewShape(const std::vector<float>& shape, int peakPosition);
    void addLinearShape(const std::vector<std::pair<int, float>>& points, int peakPosition);

private:
    std::vector<std::vector<float>> _shapes;
    int _numberOfShapes;
    int _envsize;
    std::vector<int> _peaks;

    /**
     * @brief Stretches or shrinks a curve to match a specified length.
     * 
     * @param inputCurve The original curve to stretch or shrink.
     * @param stretchedCurve Output vector to store the adjusted curve.
     * @param newLength The virtual length of the stretched curve.
	 * @param excludePeak If true, the last point of the curve is excluded from the output.
     */
	void stretchCurve(const std::vector<float>& inputCurve, std::vector<float>& stretchedCurve, float newLength, bool excludePeak = false);
};

#endif // ENVELOPESINTERPOLATOR_H