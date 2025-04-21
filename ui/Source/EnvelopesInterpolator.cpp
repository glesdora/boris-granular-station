#include "EnvelopesInterpolator.h"

EnvelopesInterpolator::EnvelopesInterpolator(int envsize) : _numberOfShapes(0), _envsize(envsize)
{
    if (_envsize == 0) return;
}

EnvelopesInterpolator::EnvelopesInterpolator(EnvelopeTable e) : _numberOfShapes(e.numberOfShapes), _envsize(e.envsize)
{
    if (_envsize == 0) return;
    if (e.peaks.size() != _numberOfShapes) return;

    _peaks = std::vector<int>(e.peaks);

    _shapes.resize(_numberOfShapes);
    for (int i = 0; i < _numberOfShapes; i++) {
        _shapes[i].resize(_envsize);
        for (int j = 0; j < _envsize; j++) {
            _shapes[i][j] = e.data[i * _envsize + j];
        }
    }
}

void EnvelopesInterpolator::interpolate(float s, float* targetbuffer)
{
    /*
        Interpolation Logic:
        1. Shapes A and B are interpolated based on a factor, `s`, where 0.0 ≤ s < _numberOfShapes.
        - The integer part of `s` determines the indices of shapes A and B between which the interpolation occurs.
            For example, if `s = 1.5`, shape A is at index 1, and shape B is at index 2.
        - The fractional part of `s` represents the interpolation factor between these shapes.
            - When the fractional part is 0, the result matches shape A.
            - When the fractional part is close to 1, the result approaches shape B.

        2. The peak position of the new shape is calculated as the weighted average of the peak positions
           of shapes A and B. 

        3. Each shape is split into two sections at its respective peak position.

        4. These sections are stretched or shrunk independently to align with the new peak position.

        5. The adjusted sections from both shapes are recombined to form complete, intermediate curves.

        6. A linear interpolation is performed between the recombined curves to produce the final shape.
    */

    if (s < 0 || s >= _numberOfShapes) return;
    
    int s_int = (int)s;
    float s_dec = s - s_int;

    // If s is an integer, return the corresponding shape
    if (s_dec == 0) {
        for (int i = 0; i < _envsize; i++) {
            targetbuffer[i] = _shapes[s_int][i];
        }
        return;
    }

    //ghost (float) peak position of new shape
    float ghost_peak_xpos;

    //left/right parts of shapes A and B (peak will be included in both)
    std::vector<float> l_A;
    std::vector<float> r_A;
    std::vector<float> l_B;
    std::vector<float> r_B;

    //new shapes A and B, stretched and shrunk to match the new peak position
    std::vector<float> stretched_A;
    std::vector<float> stretched_B;

    int i1 = s_int;
    int i2 = (s_int + 1) % _numberOfShapes;

    ghost_peak_xpos = (_peaks[i2] - _peaks[i1]) * s_dec + _peaks[i1];

	float xspan_L = ghost_peak_xpos + 1;
	float xspan_R = _envsize - ghost_peak_xpos;

	//if xspan_L is an integer, set excludePeak to true for the right parts to avoid including it twice
	bool excludePeak = (xspan_L == static_cast<int>(xspan_L));

    l_A = std::vector<float>(_shapes[i1].begin(), _shapes[i1].begin() + _peaks[i1] + 1);
    r_A = std::vector<float>(_shapes[i1].begin() + _peaks[i1], _shapes[i1].end());
    l_B = std::vector<float>(_shapes[i2].begin(), _shapes[i2].begin() + _peaks[i2] + 1);
    r_B = std::vector<float>(_shapes[i2].begin() + _peaks[i2], _shapes[i2].end());

    std::vector<float> stretched_A_L;
    std::vector<float> stretched_B_L;
    std::vector<float> stretched_A_R;
    std::vector<float> stretched_B_R;
    stretchCurve(l_A, stretched_A_L, xspan_L);
    stretchCurve(l_B, stretched_B_L, xspan_L);
	std::reverse(r_A.begin(), r_A.end());
	std::reverse(r_B.begin(), r_B.end());
	stretchCurve(r_A, stretched_A_R, xspan_R, excludePeak);
	stretchCurve(r_B, stretched_B_R, xspan_R, excludePeak);
	std::reverse(stretched_A_R.begin(), stretched_A_R.end());
	std::reverse(stretched_B_R.begin(), stretched_B_R.end());
        
    stretched_A = std::vector<float>(stretched_A_L);
    stretched_A.insert(stretched_A.end(), stretched_A_R.begin(), stretched_A_R.end());
    stretched_B = std::vector<float>(stretched_B_L);
    stretched_B.insert(stretched_B.end(), stretched_B_R.begin(), stretched_B_R.end());

    // Interpolate between stretched_A and stretched_B
    for (int i = 0; i < _envsize; i++) {
        targetbuffer[i] = (1 - s_dec) * stretched_A[i] + s_dec * stretched_B[i];
    }
}

void EnvelopesInterpolator::interpolate(float s, std::vector<float>& targetbuffer)
{
    if (targetbuffer.size() != _envsize) return;
    interpolate(s, targetbuffer.data());
}

void EnvelopesInterpolator::stretchCurve(const std::vector<float>& inputCurve, std::vector<float>& stretchedCurve, float v_len, bool excludePeak)
{
	int originalSize = static_cast<int>(inputCurve.size());
	int newSize = static_cast<int>(v_len) - static_cast<int>(excludePeak);

	stretchedCurve.resize(newSize);

    for (int x = 0; x < newSize; ++x) {
        float originalX = (x) * (originalSize - 1) / (v_len - 1);

        int x0 = static_cast<int>(originalX);
        int x1 = std::min(x0 + 1, originalSize - 1);

        float y0 = inputCurve[x0];
        float y1 = inputCurve[x1];

        float t = originalX - x0;
        stretchedCurve[x] = y0 + t * (y1 - y0);
    }
}

//set new data and peaks, with data being a one dimensional array of size numberOfShapes*envsize
void EnvelopesInterpolator::setDataAndPeaks(const float* data, const std::vector<int>& peaks)
{
    if (data == nullptr) return;
	if (peaks.size() != _numberOfShapes) return;
	for (int i = 0; i < _numberOfShapes; i++) {
		if (data[i * _envsize] != 0 || data[i * _envsize + _envsize - 1] != 0) return;
	}
    
    _numberOfShapes = peaks.size();

    _shapes.resize(_numberOfShapes);
    for (int n = 0; n < _numberOfShapes; n++) {
        _shapes[n].resize(_envsize);
        for (int i = 0; i < _envsize; i++) {
            _shapes[n][i] = data[n * _envsize + i];
        }
    }

    _peaks = std::vector<int>(peaks);
}

//set new data, peaks and envsize
void EnvelopesInterpolator::setEnvelopeTable(EnvelopeTable e)
{
    if (e.data == nullptr) return;
    if (e.numberOfShapes != e.peaks.size()) return;
	for (int i = 0; i < e.numberOfShapes; i++) {
		if (e.data[i * e.envsize] != 0 || e.data[i * e.envsize + e.envsize - 1] != 0) return;
	}

    _envsize = e.envsize;
    _numberOfShapes = e.numberOfShapes;
    _peaks = std::vector<int>(e.peaks);

    _shapes.resize(_numberOfShapes);
    for (int n = 0; n < _numberOfShapes; n++) {
        _shapes[n].resize(_envsize);
        for (int i = 0; i < _envsize; i++) {
            _shapes[n][i] = e.data[n * _envsize + i];
        }
    }
}

//add a new shape at the end of the table
void EnvelopesInterpolator::addNewShape(const std::vector<float>& shape, int peakPosition)
{
	if (shape.size() != _envsize) return;
	if (shape[0] != 0 || shape[_envsize - 1] != 0) return;

	_shapes.push_back(shape);
	_numberOfShapes++;
	_peaks.push_back(peakPosition);
}

//add a new shape, drawn via linear interpolation between given points, at the end of the table
void EnvelopesInterpolator::addLinearShape(const std::vector<std::pair<int, float>>& points, int peakPosition)
{
    if (points[0].first != 0 || points[points.size()-1].first != _envsize - 1) return;
	if (points[0].second != 0 || points[points.size() - 1].second != 0) return;

    std::vector<float> newshape(_envsize);

    for (size_t i = 0; i < points.size() - 1; ++i) {
        int x0 = points[i].first;
        float y0 = points[i].second;
        int x1 = points[i + 1].first;
        float y1 = points[i + 1].second;

        for (int x = x0; x <= x1; ++x) {
            float t = static_cast<float>(x - x0) / (x1 - x0);
            newshape[x] = y0 + t * (y1 - y0);
        }
    }

    _shapes.push_back(newshape);
    _numberOfShapes++;
    _peaks.push_back(peakPosition);
}