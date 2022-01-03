/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.h Base abstract class for the core Open, Write/Read, Advance, Close
 * functionality. Use toolkit components to build your own Engine extending this
 * class.
 * Examples of derived classes in: adios2/engine/
 *
 *  Created on: Nov 7, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_CORE_ENGINE_H_
#define ADIOS2_CORE_ENGINE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <float.h>
#include <functional> //std::function
#include <limits.h>
#include <limits> //std::numeric_limits
#include <memory> //std::shared_ptr
#include <set>
#include <string>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/core/VariableCompound.h"
#include "adios2/helper/adiosComm.h"

namespace adios2
{
namespace core
{

/** Base Abstract class for IO operations:  Read/Write, Schedule, Advance and
 * Close */
class Engine
{
public:
    using AdvanceAsyncCallback =
        std::function<void(std::shared_ptr<core::Engine>)>;

public:
    /** from derived class */
    const std::string m_EngineType;

    /** IO class object that creates this Engine at Open */
    IO &m_IO;

    /** Unique name for this Engine within m_IO */
    const std::string m_Name;

    /** open mode from ADIOSTypes.h OpenMode */
    const Mode m_OpenMode;

    /**
     * Unique Base class constructor
     * @param engineType derived class identifier
     * @param io object that generates this Engine
     * @param name unique engine name within IO class object
     * @param mode  open mode from ADIOSTypes.h Mode
     * @param comm  communicator passed at Open or from ADIOS class
     */
    Engine(const std::string engineType, IO &io, const std::string &name,
           const Mode mode, helper::Comm comm);

    virtual ~Engine();

    explicit operator bool() const noexcept;

    /**
     * Gets the factory IO object
     * @return reference to IO object that created this engine
     */
    IO &GetIO() noexcept;

    /**
     * Returns the Mode used at Open for current Engine
     * @return
     */
    Mode OpenMode() const noexcept;

    StepStatus BeginStep();

    /**
     * Indicates the beginning of a step. Typically used for streaming and
     * inside step loops.
     * @param mode stepping mode
     * @param timeoutSeconds (not yet implemented)
     * @return current step status
     */
    virtual StepStatus BeginStep(StepMode mode,
                                 const float timeoutSeconds = -1.0);

    /**
     * Returns current step information for each engine.
     * @return current step
     */
    virtual size_t CurrentStep() const;

    /**
     * Put signature that pre-allocates a Variable in Buffer returning a Span of
     * the payload memory from variable.m_Count
     * @param variable input variable to be allocated
     * @param bufferID
     * @param value
     * @return span to the buffer internal memory that be populated by the
     * application
     */
    template <class T>
    typename Variable<T>::Span &
    Put(Variable<T> &variable, const bool initialize, const T &value = T{});

    /**
     * @brief Put associates variable and data into adios2 in Engine Write mode.
     * Check your Engine documentation for specific behavior.
     * In general, it will register variable metadata and data for buffering.
     * @param variable contains metadata
     * @param data contains user defined data
     * @param executeMode
     * <pre>
     * Deferred (default): lazy evaluation, can't reuse data until EndStep
     * 		               Close, or PerformPuts.
     * Sync: data can be reused after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: always checks for user inputs
     * std::runtime_error: always checks for system failures
     * </pre>
     */
    template <class T>
    void Put(Variable<T> &variable, const T *data,
             const Mode launch = Mode::Deferred);

    /**
     * @brief Put version that accepts a variable name as input parameter.
     * Throws an exception if variable is not found in IO that created the
     * current engine.
     * @param variableName input variable name (Variable must exist in IO that
     * created current Engine with Open)
     * @param data contains user defined data
     * @param executeMode
     * <pre>
     * Deferred (default): lazy evaluation, can't reuse data until EndStep
     * 		               Close, or PerformPuts.
     * Sync: data can be reused after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: checks for user inputs, also thrown if variable is
     * not found. std::runtime_error: always checks for system failures
     * </pre>
     */
    template <class T>
    void Put(const std::string &variableName, const T *data,
             const Mode launch = Mode::Deferred);

    /**
     * Put version for single value datum, can accept on-the-fly values
     * e.g. Put<float>(variable, 10.f);
     * Mode is always Sync since there might be no pointer associated with the
     * single value datum (r-values)
     * @param variable contains metadata
     * @param datum contains user defined single value
     */
    template <class T>
    void Put(Variable<T> &variable, const T &datum, const Mode launch);

    /**
     * @brief Put version for single value datum using variable name. Throws
     * an exception if variable is not found in IO that created the
     * current engine.
     *
     * Can accept on-the-fly values e.g. Put<float>("myVar", 10.f);
     * Mode is always Sync since there might be no pointer associated with
     * the single value datum (r-values)
     *
     * @param variableName input variable name (Variable must exist in IO that
     * created current Engine with Open)
     * @param datum contains user defined single value
     * @exception
     * <pre>
     * std::invalid_argument: checks for user inputs also thrown if variable is
     * not found. std::runtime_error: always checks for system failures
     * </pre>
     */
    template <class T>
    void Put(const std::string &variableName, const T &datum,
             const Mode launch);

    /**
     * @brief Get associates an existing variable selections and populates data
     * from adios2 Engine in Read Mode.
     *
     * Polymorphic function.
     * Check your Engine documentation for specific behavior.
     * In general, it will register variable metadata and data for populating
     * data values at Read.
     * @param variable contains metadata and selections for getting the variable
     * @param data user pre-allocated memory space
     * @param executeMode
     * <pre>
     * Deferred (default): lazy evaluation, data is not populated until EndStep
     * 		Close, or PerformPuts
     * Sync: data is ready after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: checks for user inputs
     * std::runtime_error: always if system failures are caught
     * </pre>
     */
    template <class T>
    void Get(Variable<T> &variable, T *data,
             const Mode launch = Mode::Deferred);

    /**
     * @brief Get version that accepts a variableName as input.
     *
     * Throws an exception if variable is not found in IO that created the
     * current engine.
     *
     * @param variableName input variable name (Variable must exist in IO that
     * created current Engine with Open)
     * @param data user pre-allocated memory space
     * @param executeMode
     * <pre>
     * Deferred (default): lazy evaluation, data is not populated until EndStep
     * 		Close, or PerformPuts.
     * Sync: data is ready after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: checks for user inputs, also thrown if variable is
     * not found. std::runtime_error: always if system failures are caught
     * </pre>
     */
    template <class T>
    void Get(const std::string &variableName, T *data,
             const Mode launch = Mode::Deferred);

    /**
     * @brief Get version for single value datum
     * Mode is always Sync since there might be no pointer associated with
     * the
     * single value datum (r-values)
     * @param variable contains metadata
     * @param datum to be populated with corresponding value
     */
    template <class T>
    void Get(Variable<T> &variable, T &datum,
             const Mode launch = Mode::Deferred);

    /**
     * @brief Get version for single value datum using variable name
     * @param variableName input variable name (Variable must exist in IO that
     * created current Engine with Open)
     * @param datum to be populated with corresponding value
     */
    template <class T>
    void Get(const std::string &variableName, T &datum,
             const Mode launch = Mode::Deferred);

    /**
     * Convenience function, C++ only that allocates and populates a vector with
     * the requested values
     * @param variable
     * @param dataV
     * @param launch
     */
    template <class T>
    void Get(Variable<T> &variable, std::vector<T> &dataV,
             const Mode launch = Mode::Deferred);

    /**
     * Convenience function, C++ only that allocates and populates a vector with
     * the requested values
     * @param variable
     * @param dataV
     * @param launch
     */
    template <class T>
    void Get(const std::string &variableName, std::vector<T> &dataV,
             const Mode launch = Mode::Deferred);

    /**
     * @brief Get version retrieves an existing variable's block selections and
     * sets the input data pointer
     * from adios2 Engine Write mode directly to Read Mode. If the data is not
     * available (likely for all Engines except Inline), return null, or TODO
     * allocate and fill in a buffer.
     *
     * Polymorphic function.
     * Check your Engine documentation for specific behavior.
     * In general, it will register variable metadata and data for populating
     * data values at Read.
     * @param variable contains metadata and selections for getting the variable
     * @param executeMode
     * @return pointer to variable's block info for this block selection.
     * <pre>
     * Deferred (default): lazy evaluation, data is not populated until EndStep
     *      Close, or PerformPuts
     * Sync: data is ready after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: checks for user inputs
     * std::runtime_error: always if system failures are caught
     * </pre>
     */
    template <class T>
    typename Variable<T>::BPInfo *Get(Variable<T> &variable,
                                      const Mode launch = Mode::Deferred);

    /**
     * @brief Get version for block selection that accepts a variableName as
     * input.
     *
     * Throws an exception if variable is not found in IO that created the
     * current engine.
     *
     * @param variableName input variable name (Variable must exist in IO that
     * created current Engine with Open)
     * @param executeMode
     * @return pointer to variable's block info for this block selection.
     * <pre>
     * Deferred (default): lazy evaluation, data is not populated until EndStep
     *      Close, or PerformPuts.
     * Sync: data is ready after this call
     * </pre>
     * @exception
     * <pre>
     * std::invalid_argument: checks for user, also thrown if variable is not
     * found.
     * std::runtime_error: always if system failures are caught
     * </pre>
     */
    template <class T>
    typename Variable<T>::BPInfo *Get(const std::string &variableName,
                                      const Mode launch = Mode::Deferred);

    template <class T>
    void Get(core::Variable<T> &, T **) const;

    /**
     * Reader application indicates that no more data will be read from the
     * current stream before advancing.
     * This is necessary to allow writers to advance as soon as possible.
     */
    virtual void EndStep();

    /** Execute all Put (in deferred launch mode) starting from a previous
     * PerformPuts, BeginStep or Open */
    virtual void PerformPuts();

    /** Execute all Get (in deferred launch mode) starting from a previous
     * PerformGets, BeginStep or Open */
    virtual void PerformGets();

    /**
     * Closes a particular transport, or all if transportIndex = -1 (default).
     * @param transportIndex index returned from IO AddTransport, default (-1) =
     * all
     */
    void Close(const int transportIndex = -1);

    /**
     * Flushes data and metadata (if on) to a particular transport, or all if -1
     * (default).
     * @param transportIndex index returned from IO AddTransport, default (-1) =
     * all
     */
    virtual void Flush(const int transportIndex = -1);

    /**
     * Extracts all available blocks information for a particular
     * variable. This can be an expensive function, memory scales up with
     * metadata: steps and blocks per step
     * Valid in read mode only.
     * @param variable
     * @return map with all variable blocks information
     * <pre>
     * 	  key: step
     * 	  value: vector of blocks with info for each block per step
     * </pre>
     */
    template <class T>
    std::map<size_t, std::vector<typename Variable<T>::BPInfo>>
    AllStepsBlocksInfo(const Variable<T> &variable) const;

    /**
     * This function is internal, for public interface use
     * Variable<T>::AllStepsBlocksInfo
     * @param variable
     * @return
     */
    template <class T>
    std::vector<std::vector<typename Variable<T>::BPInfo>>
    AllRelativeStepsBlocksInfo(const Variable<T> &variable) const;

    /**
     * Extracts all available blocks information for a particular
     * variable and step.
     * Valid in read mode only.
     * @param variable input variable
     * @param step input from which block information is extracted
     * @return vector of blocks with info for each block per step, if step not
     * found it returns an empty vector
     */
    template <class T>
    std::vector<typename Variable<T>::BPInfo>
    BlocksInfo(const Variable<T> &variable, const size_t step) const;

    /**
     * Get the absolute steps of a variable in a file. This is for
     * information purposes only, because absolute steps cannot be used
     * in any ADIOS2 calls.
     */
    template <class T>
    std::vector<size_t> GetAbsoluteSteps(const Variable<T> &variable) const;

    template <class T>
    T *BufferData(const int bufferIdx, const size_t payloadOffset,
                  const size_t bufferID = 0) noexcept;

    size_t Steps() const;

    /**
     * @brief Promise that no more definitions or changes to defined variables
     * will occur. Useful information if called before the first EndStep() of an
     * output Engine, as it will know that the definitions are complete and
     * constant for the entire lifetime of the output and may optimize metadata
     * handling.
     */
    void LockWriterDefinitions() noexcept;

    /**
     * @brief Promise that the reader data selections of are fixed and
     * will not change in future timesteps. This information, provided
     * before the EndStep() representing a fixed read pattern, may be
     * utilized by the input Engine to optimize data flow.
     */
    void LockReaderSelections() noexcept;

    /* for adios2 internal testing */
    virtual size_t DebugGetDataBufferSize() const;

    union PrimitiveStdtypeUnion
    {
#define declare_field(T, N) T field_##N;
        ADIOS2_FOREACH_MINMAX_STDTYPE_2ARGS(declare_field)
#undef declare_field
    };

    struct MinMaxStruct
    {
        union PrimitiveStdtypeUnion MinUnion;
        union PrimitiveStdtypeUnion MaxUnion;
        void Init(DataType Type)
        {
            memset(this, 0, sizeof(struct MinMaxStruct));
            switch (Type)
            {
            case DataType::None:
                break;
            case DataType::Int8:
                MinUnion.field_int8 = INT8_MAX;
                MaxUnion.field_int8 = INT8_MIN;
                break;
            case DataType::Int16:
                MinUnion.field_int16 = INT16_MAX;
                MaxUnion.field_int16 = INT16_MIN;
                break;
            case DataType::Int32:
                MinUnion.field_int32 = INT32_MAX;
                MaxUnion.field_int32 = INT32_MIN;
                break;
            case DataType::Int64:
                MinUnion.field_int64 = INT64_MAX;
                MaxUnion.field_int64 = INT64_MIN;
                break;
            case DataType::Char:
            case DataType::UInt8:
                MinUnion.field_uint8 = UINT8_MAX;
                MaxUnion.field_uint8 = 0;
                break;
            case DataType::UInt16:
                MinUnion.field_uint16 = UINT16_MAX;
                MaxUnion.field_uint16 = 0;
                break;
            case DataType::UInt32:
                MinUnion.field_uint32 = UINT32_MAX;
                MaxUnion.field_uint32 = 0;
                break;
            case DataType::UInt64:
                MinUnion.field_uint64 = UINT64_MAX;
                MaxUnion.field_uint64 = 0;
                break;
            case DataType::Float:
                MinUnion.field_float = FLT_MAX;
                MaxUnion.field_float = -FLT_MAX;
                break;
            case DataType::Double:
                MinUnion.field_double = DBL_MAX;
                MaxUnion.field_double = -DBL_MAX;
                break;
            case DataType::LongDouble:
                MinUnion.field_ldouble = LDBL_MAX;
                MaxUnion.field_ldouble = -LDBL_MAX;
                break;
            case DataType::FloatComplex:
            case DataType::DoubleComplex:
            case DataType::String:
            case DataType::Compound:
                break;
            }
        }
        void Dump(DataType Type)
        {
            switch (Type)
            {
            case DataType::None:
                break;
            case DataType::Int8:
                std::cout << "Min : " << MinUnion.field_int8
                          << ", Max : " << MaxUnion.field_int8;
                break;
            case DataType::Int16:
                std::cout << "Min : " << MinUnion.field_int16
                          << ", Max : " << MaxUnion.field_int16;
                break;
            case DataType::Int32:
                std::cout << "Min : " << MinUnion.field_int32
                          << ", Max : " << MaxUnion.field_int32;
                break;
            case DataType::Int64:
                std::cout << "Min : " << MinUnion.field_int64
                          << ", Max : " << MaxUnion.field_int64;
                break;
            case DataType::Char:
            case DataType::UInt8:
                std::cout << "Min : " << MinUnion.field_uint8
                          << ", Max : " << MaxUnion.field_uint8;
                break;
            case DataType::UInt16:
                std::cout << "Min : " << MinUnion.field_uint16
                          << ", Max : " << MaxUnion.field_uint16;
                break;
            case DataType::UInt32:
                std::cout << "Min : " << MinUnion.field_uint32
                          << ", Max : " << MaxUnion.field_uint32;
                break;
            case DataType::UInt64:
                std::cout << "Min : " << MinUnion.field_uint64
                          << ", Max : " << MaxUnion.field_uint64;
                break;
            case DataType::Float:
                std::cout << "Min : " << MinUnion.field_float
                          << ", Max : " << MaxUnion.field_float;
                break;
            case DataType::Double:
                std::cout << "Min : " << MinUnion.field_double
                          << ", Max : " << MaxUnion.field_double;
                break;
            case DataType::LongDouble:
                std::cout << "Min : " << MinUnion.field_ldouble
                          << ", Max : " << MaxUnion.field_ldouble;
                break;
            case DataType::FloatComplex:
            case DataType::DoubleComplex:
            case DataType::String:
            case DataType::Compound:
                break;
            }
        }
    };
    struct MinBlockInfo
    {
        int WriterID = 0;
        size_t BlockID = 0;
        size_t *Start;
        size_t *Count;
        MinMaxStruct MinMax;
        void *BufferP = NULL;
    };
    struct MinVarInfo
    {
        int Dims;
        size_t *Shape;
        bool IsValue = false;
        bool IsReverseDims = false;
        std::vector<struct MinBlockInfo> BlocksInfo;
        MinVarInfo(int D, size_t *S)
        : Dims(D), Shape(S), IsValue(false), IsReverseDims(false),
          BlocksInfo({})
        {
        }
    };

    //  in this call, Step is RELATIVE, not absolute
    virtual MinVarInfo *MinBlocksInfo(const VariableBase &,
                                      const size_t Step) const
    {
        return nullptr;
    }

    virtual bool VariableMinMax(const VariableBase &, const size_t Step,
                                MinMaxStruct &MinMax)
    {
        return false;
    }

    /** Notify the engine when a new attribute is defined. Called from IO.tcc
     */
    virtual void NotifyEngineAttribute(std::string name,
                                       DataType type) noexcept;

    /** Inform about computation block through User->ADIOS->IO */
    virtual void EnterComputationBlock() noexcept;
    /** Inform about computation block through User->ADIOS->IO */
    virtual void ExitComputationBlock() noexcept;

protected:
    /** from ADIOS class passed to Engine created with Open
     *  if no communicator is passed */
    helper::Comm m_Comm;

    /** keeps track of current advance status */
    StepStatus m_AdvanceStatus = StepStatus::OK;

    /** keep track if the current Engine is marked for destruction in IO */
    bool m_IsClosed = false;

    /** carries the number of available steps in each Engine */
    size_t m_Steps = 0;

    /** Called from constructors */
    virtual void Init();

    /** From IO SetParameters */
    virtual void InitParameters();

    /** From IO AddTransport */
    virtual void InitTransports();

// Put
#define declare_type(T)                                                        \
    virtual void DoPut(Variable<T> &variable,                                  \
                       typename Variable<T>::Span &span,                       \
                       const bool initialize, const T &value);
    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T)                                                        \
    virtual void DoPutSync(Variable<T> &, const T *);                          \
    virtual void DoPutDeferred(Variable<T> &, const T *);
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

// Get
#define declare_type(T)                                                        \
    virtual void DoGetSync(Variable<T> &, T *);                                \
    virtual void DoGetDeferred(Variable<T> &, T *);                            \
    virtual typename Variable<T>::BPInfo *DoGetBlockSync(Variable<T> &);       \
    virtual typename Variable<T>::BPInfo *DoGetBlockDeferred(Variable<T> &);
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    virtual void DoClose(const int transportIndex) = 0;

    /**
     * Called by string Put/Get versions and deferred modes
     * @param variableName variable to be searched
     * @param hint extra exception information
     * @return Variable<T>& reference if found, else throws an exception
     */
    template <class T>
    Variable<T> &FindVariable(const std::string &variableName,
                              const std::string hint);

#define declare_type(T)                                                        \
    virtual std::map<size_t, std::vector<typename Variable<T>::BPInfo>>        \
    DoAllStepsBlocksInfo(const Variable<T> &variable) const;                   \
                                                                               \
    virtual std::vector<std::vector<typename Variable<T>::BPInfo>>             \
    DoAllRelativeStepsBlocksInfo(const Variable<T> &variable) const;           \
                                                                               \
    virtual std::vector<typename Variable<T>::BPInfo> DoBlocksInfo(            \
        const Variable<T> &variable, const size_t step) const;                 \
    virtual std::vector<size_t> DoGetAbsoluteSteps(                            \
        const Variable<T> &variable) const;

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T, L)                                                     \
    virtual T *DoBufferData_##L(const int bufferIdx,                           \
                                const size_t payloadPosition,                  \
                                const size_t bufferID) noexcept;

    ADIOS2_FOREACH_PRIMITVE_STDTYPE_2ARGS(declare_type)
#undef declare_type

    virtual size_t DoSteps() const;

    /** true: No more definitions or changes to existing variables are allowed
     */
    bool m_WriterDefinitionsLocked = false;

    /** true: The read pattern is fixed and will not change.
     */
    bool m_ReaderSelectionsLocked = false;

    /** true: Currently executing after BeginStep and before EndStep
     */
    bool m_BetweenStepPairs = false;

private:
    /** Throw exception by Engine virtual functions not implemented/supported by
     *  a derived  class */
    void ThrowUp(const std::string function) const;

    /**
     * Execute common checks in Put and Get
     * @param variable input variable
     * @param data input data
     * @param modes acceptable modes
     * @param hint extra exception info
     */
    template <class T>
    void CommonChecks(Variable<T> &variable, const T *data,
                      const std::set<Mode> &modes,
                      const std::string hint) const;

    /**
     * Checks if Engine was opened using the right Open mode for a particular
     * function
     * @param modes acceptable modes
     * @param hint extra exception info
     */
    void CheckOpenModes(const std::set<Mode> &modes,
                        const std::string hint) const;
};

#define declare_template_instantiation(T)                                      \
                                                                               \
    extern template void Engine::Put<T>(Variable<T> &, const T *, const Mode); \
    extern template void Engine::Put<T>(const std::string &, const T *,        \
                                        const Mode);                           \
                                                                               \
    extern template void Engine::Put<T>(Variable<T> &, const T &, const Mode); \
    extern template void Engine::Put<T>(const std::string &, const T &,        \
                                        const Mode);                           \
                                                                               \
    extern template void Engine::Get<T>(Variable<T> &, T *, const Mode);       \
    extern template void Engine::Get<T>(const std::string &, T *, const Mode); \
                                                                               \
    extern template void Engine::Get<T>(Variable<T> &, T &, const Mode);       \
    extern template void Engine::Get<T>(const std::string &, T &, const Mode); \
                                                                               \
    extern template void Engine::Get<T>(Variable<T> &, std::vector<T> &,       \
                                        const Mode);                           \
                                                                               \
    extern template void Engine::Get<T>(const std::string &, std::vector<T> &, \
                                        const Mode);                           \
                                                                               \
    extern template typename Variable<T>::BPInfo *Engine::Get<T>(              \
        Variable<T> &, const Mode);                                            \
    extern template typename Variable<T>::BPInfo *Engine::Get<T>(              \
        const std::string &, const Mode);                                      \
                                                                               \
    extern template Variable<T> &Engine::FindVariable(                         \
        const std::string &variableName, const std::string hint);              \
                                                                               \
    extern template std::map<size_t,                                           \
                             std::vector<typename Variable<T>::BPInfo>>        \
    Engine::AllStepsBlocksInfo(const Variable<T> &) const;                     \
                                                                               \
    extern template std::vector<std::vector<typename Variable<T>::BPInfo>>     \
    Engine::AllRelativeStepsBlocksInfo(const Variable<T> &) const;             \
                                                                               \
    extern template std::vector<typename Variable<T>::BPInfo>                  \
    Engine::BlocksInfo(const Variable<T> &, const size_t) const;               \
                                                                               \
    extern template std::vector<size_t> Engine::GetAbsoluteSteps(              \
        const Variable<T> &) const;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    extern template typename Variable<T>::Span &Engine::Put(                   \
        Variable<T> &, const bool, const T &);                                 \
    extern template void Engine::Get(Variable<T> &, T **) const;
ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_CORE_ENGINE_H_ */
