#include "network_fields.hpp"

// The Macro sets' and procedures' field tables (D570–D573, D636), moved out of network_fields.cpp
// for its line limit. Every row here is closed; fields() and allowsAdditionalFields() delegate.
namespace multra::networkCodec {
namespace {
// D570: trip_rate_codec.cpp's rows (all closed). A side is one of two rows chosen by `method`; the
// codec's other rules (unique ids and attributes, nonblank text, nonempty categories) stay with it.
constexpr Field tripRateSetFields[]={{"id",Kind::text,true},{"name",Kind::text,true},{"source",Kind::citedSource,true},
    {"unit",Kind::text,true},{"purposes",Kind::tripRatePurposes,true}};
// D574: the cited source shared by trip-rate, deterrence and mode-choice sets (a cost matrix's
// source holds citation only). A key added here is legal in all three readers.
constexpr Field citedSourceFields[]={{"citation",Kind::text},{"table",Kind::text}};
constexpr Field tripRatePurposeFields[]={{"id",Kind::text,true},{"productions",Kind::tripRateSide,true},{"attractions",Kind::tripRateSide,true}};
constexpr Field tripRateCrossFields[]={{"method",Kind::crossMethod,true},{"categories",Kind::tripRateCategories,true}};
constexpr Field tripRateRegressionFields[]={{"method",Kind::regressionMethod,true},{"constant",Kind::number,true},{"terms",Kind::tripRateTerms,true}};
constexpr Field tripRateCategoryFields[]={{"attribute",Kind::text,true},{"rate",Kind::rate,true}};
constexpr Field tripRateTermFields[]={{"attribute",Kind::text,true},{"coefficient",Kind::number,true}};
// D571: deterrence_codec.cpp and cost_matrix_codec.cpp's closed rows. Deterrence reuses the
// trip-rate source row; a matrix source permits citation only. Blank/unique ids and n-by-n
// agreement stay with the codecs. The document owns the constraint floors.
constexpr Field deterrenceSetFields[]={{"id",Kind::text,true},{"name",Kind::text,true},{"source",Kind::citedSource,true},
    {"function",Kind::exponentialFunction,true},{"costUnit",Kind::minuteUnit,true},{"purposes",Kind::deterrencePurposes,true}};
constexpr Field deterrencePurposeFields[]={{"id",Kind::text,true},{"beta",Kind::deterrenceBeta,true},
    {"intrazonalShare",Kind::intrazonalShare},{"constraint",Kind::deterrenceConstraint,false,0,constraintVersion}};
constexpr Field costMatrixFields[]={{"id",Kind::text,true},{"name",Kind::text,true},{"source",Kind::costMatrixSource},
    {"unit",Kind::minuteUnit,true},{"zoneIds",Kind::nonemptyTexts,true},{"rows",Kind::costRows,true}};
constexpr Field costMatrixSourceFields[]={{"citation",Kind::text}};
// D572: period_factor_codec.cpp's exact row; blank ids, the share sum, known periods and unique pairs stay with the codec.
constexpr Field periodFactorFields[]={{"purposeId",Kind::text,true},{"periodId",Kind::text,true},
    {"productionToAttraction",Kind::periodShare,true},{"attractionToProduction",Kind::periodShare,true}};
// D573: mode_choice_codec.cpp's closed rows; source reuses citedSource. Semantic checks stay in the codec.
constexpr Field modeChoiceSetFields[]={{"id",Kind::text,true},{"name",Kind::text,true},{"source",Kind::citedSource,true},
    {"costUnit",Kind::minuteUnit,true},{"purposes",Kind::modeChoicePurposes,true}};
constexpr Field modeChoicePurposeFields[]={{"id",Kind::text,true},{"alternatives",Kind::modeChoiceAlternatives,true},
    {"nests",Kind::modeChoiceNests,false,0,modeNestVersion},{"distributionCost",Kind::logsumCost,false,0,logsumVersion}};
constexpr Field modeChoiceAlternativeFields[]={{"id",Kind::text,true},{.name="modeId",.kind=Kind::text,.atLeastOne=true},
    {.name="costMatrix",.kind=Kind::text,.atLeastOne=true},
    {"constant",Kind::number,true},{"timeCoefficient",Kind::modeCoefficient,true},{"occupancy",Kind::modeOccupancy}};
constexpr Field modeChoiceNestFields[]={{"id",Kind::text,true},{"phi",Kind::modeNestPhi,true},{"alternatives",Kind::nonemptyTexts,true}};// D636: procedure_codec.cpp's rows, read with onlyKnownKeys. A step is one of four rows chosen by
// `kind`; `period` is required unless `macroRun` is present and refused when it is. Blank and unique
// ids and every reference stay with the codec and the run. Feedback's method-dependent weight is a
// condition in network_schema.cpp; its bounds are checked after parsing (feedbackIssues).
constexpr Field procedureFields[]={{"id",Kind::text,true},{"name",Kind::text,true},{"steps",Kind::procedureSteps,true},
    {"feedback",Kind::procedureFeedback,false,0,feedbackVersion}};
constexpr Field generationStepFields[]={{"kind",Kind::stepKind,true},{"active",Kind::boolean},{"rateSet",Kind::text,true}};
constexpr Field distributionStepFields[]={{"kind",Kind::stepKind,true},{"active",Kind::boolean},{"deterrenceSet",Kind::text,true},
    {"skimMode",Kind::text},{"costMatrix",Kind::text}};
constexpr Field modeChoiceStepFields[]={{"kind",Kind::stepKind,true},{"active",Kind::boolean},{"modeChoiceSet",Kind::text,true},
    {"periods",Kind::nonemptyTexts}};
constexpr Field assignmentStepFields[]={{"kind",Kind::stepKind,true},{"active",Kind::boolean},
    {.name="period",.kind=Kind::text,.required=true,.unless="macroRun"},{.name="macroRun",.kind=Kind::text}};
// weight in [0, 1) and tolerance ≥ 0 share the intrazonal-share and rate rules.
constexpr Field procedureFeedbackFields[]={{"method",Kind::feedbackMethod,true},{"weight",Kind::intrazonalShare},
    {"maxIterations",Kind::feedbackIterations,true},{"tolerance",Kind::rate,true}};
}
std::span<const Field> macroSetFields(Object object) {
    switch(object) {
    case Object::tripRateSet:return tripRateSetFields;
    case Object::citedSource:return citedSourceFields;
    case Object::tripRatePurpose:return tripRatePurposeFields;
    case Object::tripRateCrossSide:return tripRateCrossFields;
    case Object::tripRateRegressionSide:return tripRateRegressionFields;
    case Object::tripRateCategory:return tripRateCategoryFields;
    case Object::tripRateTerm:return tripRateTermFields;
    case Object::deterrenceSet:return deterrenceSetFields;
    case Object::deterrencePurpose:return deterrencePurposeFields;
    case Object::costMatrix:return costMatrixFields;
    case Object::costMatrixSource:return costMatrixSourceFields;
    case Object::periodFactor:return periodFactorFields;
    case Object::modeChoiceSet:return modeChoiceSetFields;
    case Object::modeChoicePurpose:return modeChoicePurposeFields;
    case Object::modeChoiceAlternative:return modeChoiceAlternativeFields;
    case Object::modeChoiceNest:return modeChoiceNestFields;
    case Object::procedure:return procedureFields;
    case Object::generationStep:return generationStepFields;
    case Object::distributionStep:return distributionStepFields;
    case Object::modeChoiceStep:return modeChoiceStepFields;
    case Object::assignmentStep:return assignmentStepFields;
    case Object::procedureFeedback:return procedureFeedbackFields;
    default:return {};
    }
}
}
