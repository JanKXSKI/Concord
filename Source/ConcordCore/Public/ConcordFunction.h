// Copyright 2022 Jan Klimaschewski. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <cmath>

using std::fmod;
using std::min;
using std::max;
using std::abs;
using std::acos;
using std::asin;
using std::atan;
using std::atan2;
using std::cos;
using std::cosh;
using std::erf;
using std::exp;
using std::hypot;
using std::log;
using std::log2;
using std::log10;
using std::pow;
using std::sin;
using std::sinh;
using std::sqrt;
using std::tan;
using std::tanh;

template<typename FValue> auto ConcordCoalesce(FValue A, FValue B) { return A ? A : B; }
inline int32 ConcordModulo(int32 A, int32 B) { return A % B; }
template<typename FValue> auto ConcordModulo(FValue A, FValue B) { return fmod(A, B); }
template<typename FValue> int32 ConcordTrunc(FValue A) { return FMath::TruncToInt(A); }
template<typename FValue> int32 ConcordRound(FValue A) { return FMath::RoundToInt(A); }
