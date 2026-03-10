# RetryPP

```RetryPP``` is a C++20 header-only library that provides a standardized way defining and performing retries on user-defined
operations. It relies on the concept of a ```Policy``` that describes how to retry an operation along with a
```Classifier``` that is responsible for classifying the result of the operation as a success,
temporary failure (retryable) or permanent failure (non-retryable failure).

All classes and functions relating to RetryPP lives in the ```RetryPP``` namespace.

Because ```RetryPP``` is a header-only implementation you can just add the ```include``` folder to your project and start
benefitting today.


## Requirements

```RetryPP``` does not have external dependencies and only require a standard-compliant C++20 compiler. It's meant to be
cross-platform.


## Policies

The ```Policy``` defines the retry ```Strategy``` to determine the retry delay and can also define zero or more ```Modifier```s
to the delay returned by the selected strategy and finally aborts retrying based on the policy's defined ```Limit```.

RetryPP uses the Builder pattern to construct ```Policy``` objects. Once created, the ```Policy``` is immutable, thread-safe and can
be re-used as many times as you like.

The ```PolicyBuilder``` can be reused to create multiple ```Policy`` objects from the same builder.

## Classifiers

The ```Classifier``` template is used to determine whether an operation should be retried or not. It is always used in combination
with a ```Policy``` and uses the result of an operation to classify it as a success, a temporary failure (retryable failure)
or a permanent (non-retryable) failure.

The type declared in the instantiation must match the return type of the operation being retried.

In order for the ```Classifier``` to function it must be able to perform compare operations on the return type of the operation. This
requires that the return type has a specialization of std::less<T> or that a custom comparator is provided.

The ```Classifier``` allows you to specify either individual codes or ranges of codes for each category of success,
temporary failure or permanent failure.

Individual values have precedence over ranges. This means you can add a successful range and treat some codes as transient or
permanent even though they fall into the successful range or vice-versa.

RetryPP uses the Builder pattern to construct ```Classifier``` objects. Once created, the ```Classifier``` is immutable, thread-safe and
can be re-used as many times as you like.

The ```ClassifierBuilder``` can be reused to create multiple ```Classifier``` objects from the same builder.


## Retry Strategies
RetryPP uses retry strategies to determine the desired retry interval and potential backoff. The following retry strategies are supplied:


### Exponential
The ```Exponential``` strategy performs exponential backoff with a multiplier using the formula
```(initial delay) * 2^(multiplier*attempt)```.

The multiplier must be a value >= 1.0f and defaults to 2.0f.


### Linear
The ```Linear``` strategy performs linear backoff using the formula
```(initial_delay) + (attempt * (initial delay))```.


### Fixed
The ```Fixed``` strategy runs with a fixed interval and no backoff using the formula
```(initial delay)```.


### Immediate
The ```Immediate``` strategy will not delay retries and does not perform backoff. This is not recommended as is mostly supplied
for completenes.
This strategy will always return a delay of 0ms.


## Retry Modifiers
RetryPP allows you to apply zero or more modifiers to the delay returned by the selected retry strategy. Modifiers are applied to
the delay returned by the retry strategy in the order they're added.

The following modifiers are supplied:


### Jitter
The ```Jitter``` modifier applies jitter to the retry ```delay``` returned by the retry strategy. The ```Jitter``` modifier uses
a ```JitterAlgorithm``` to determine the algorithm applied.
The following algorithms are supplied:


#### Full
The ```Full``` algorithm applies jitter using the formula ```random_between(0, delay)```.


#### Equal
The ```Equal``` algorithm applies jitter using the formula ```(delay / 2) + random_between(0, delay / 2)```


#### Decorrelated
The ```Decorrelated``` algorithm applies jitter using the formula ```random_between((initial delay), (previous delay) * 3)```.


This algorithm is decribed in the AWS article [Exponential Backoff and Jitter](https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/).


### Cap
The ```Cap``` modifier limits the retry interval returned by the retry strategy to a set value. This prevents retry delays
to grow unproportionately.


## Retry Limits
Limits determine when RetryPP aborts retrying. The following limits are supplied:


### RetryLimit
The ```RetryLimit``` will abort after number of attempts defined in it's constructor.


### TimeLimit
The ```TimeLimit``` will abort after (approximately) the amount of time defined in it's constructor. If a delay returned by
the retry strategy (and modifiers) extends beyond the defained timeout, the delay is shortened to whatever is left of the timeout
and a final retry is done.


### NoLimit
The ```NoLimit``` will allow retrying infinitely. It should not be used and is only included for completeness.


# Usage


## Build a Policy
RetryPP uses the Builder pattern to construct policies. Each ```Policy``` must have an associated ```Strategy``` and ```Limit```.
The use of ```Modifier```s is optional but recommended.

Use a ```PolicyBuilder``` to build a ```Policy```:

```cpp
#include <RetryPP/Policy.h>
#include <RetryPP/Backoff/Exponential.h>
#include <RetryPP/Backoff/Modifier/Cap.h>
#include <RetryPP/Limit/RetryLimit.h>

int main()
{
    using namespace std::chrono_literals;
    using namespace RetryPP;

    Policy policy = PolicyBuilder()
        .withStrategy<Exponential>(200ms) // Use the Exponential strategy with a 200 millsecond initial delay
        .withModifier<Cap>(10s) // Cap retry delays to 10 seconds
        .withLimit<RetryLimit>(3) // Don't allow more than 3 attempts
        .build();

    return 0;
}
```

The example constructs a ```Policy``` with an exponential backoff strategy starting with a 200 millisecond delay and a limit of
3 attempts. The ```Cap``` modifier ensures that the retry delay never exceed 10 seconds.


## Build a Classifier
RetryPP uses the Builder pattern to construct classifiers. Each ```Classifier``` must have at least one success code defined.

Use a ```ClassifierBuilder``` to build a ```Classifier```:

```cpp
#include <RetryPP/Classifier.h>

using HttpResponseCode = int;

int main()
{
    using namespace std::chrono_literals;
    using namespace RetryPP;

    Classifier<HttpResponseCode> httpClassifier = ClassifierBuilder<HttpResponseCode>()
        .withSuccessRange(200, 399)
        .withUndefinedCodeClassification(Classification::Permanent)
        .build();

    return 0;
}
```

The example constructs a ```Classifier<HttpResponseCode>``` that will classify HTTP response codes 200 through 399 (both inclusive) as a
successful response. Any undefined code will be treated as a permanent error.

The ```ClassifierBuilder``` supports 3 different ways to add codes for successful, transient and permanent codes:

```cpp
    with[Success|Transient|Permanent]Code(const Code& code);
```
This version allows adding a single individual code.

```cpp
    with[Success|Transient|Permanent]Codes(const std::set<Code>& set);
```
This version allows adding a number of individual codes at once.

```cpp
    with[Success|Transient|Permanent]Range(const Code& start, const Code& end);
```
This version adds a range of codes (from ```start``` to ```end``` both inclusive).

Individual values have precedence over ranges. This means you can add a successful range and treat some codes as transient or
permanent even though they fall into the successful range:

```cpp
    Classifier<HttpResponseCode> httpClassifier = ClassifierBuilder<HttpResponseCode>()
        .withSuccessRange(200, 399) // Treat response codes 200 through 399 as successful.
        .withTransientCode(208) // Treat '208 Already Reported' as a transient failure even though it lies in the range of successful values defined above.
        .withPermanentCodes({ 307, 308 }) // Treat '307 Temporary Redirect' and '308 Permanent Redirect' as permanent failures even though it lies in the range of successful values defined above.
        .withUndefinedCodeClassification(Classification::Permanent) // Any code that is not added above will be treated as a permanent error.
        .build();
```

## Use RetryPP to retry an operation

Once you have a ```Policy```and a ```Classifier``` you can retry any operation by invoking one of the ```withRetry<T>(...)``` overloads.

```cpp

using HttpResponseCode = int;

HttpResponseCode my_operation(HttpResponseCode code)
{
    return code;
}

int main()
{
    using namespace std::chrono_literals;
    using namespace RetryPP;

    // Build your Policy
    Policy policy = PolicyBuilder()
        .withStrategy<Exponential>(100ms) // Use the Exponential strategy with a 100 millsecond initial delay
        .withModifier<Jitter<Algorithm::Full>>() // Use Full Jitter modifier to randomize delay
        .withModifier<Cap>(10s)
        .withLimit<RetryLimit>(5)
        .build();

    /// Build your Classifier
    Classifier<HttpResponseCode> http_classifier = ClassifierBuilder<HttpResponseCode>()
        .withSuccessRange(100, 399)
        .withTransientCodes({ 408, 429, 500, 502, 503, 504 }) // These codes should be treated as transient failures (retryable)
        .withUndefinedCodeClassification(Classification::Permanent) // Any other code should be treated as a permanent failure (non-retryable)
        .build();

    // Retry my_operation(HttpResponseCode) using supplied policy and classifier.
    RetryResult result = withRetry(policy, classifier, &my_operation, code);
    if (result.classification != Classification::Success)
        ...
    ...
}
```


# Exception handling
```RetryPP``` supports operations that throw exceptions. If your operation can throw exceptions you should add an exception
classifier callback when building your ```Classifier```.

If the ```Classifier``` does not have an exception classifier and the operation
throws an exception, it will be treated as a permanent failure and the exception will be passed on to the caller.

The exception classifier must return a ```Classification``` for the exception passed to it. If the exception classifier
returns ```Classification::Permanent``` the exception will be floated on to the caller. Any other return value will
be treated as a transient failure and cause the operation to be retried in accordance with the supplied ```Policy```.

If the operation throws an exception and retries are exhausted the exception will be passed on to the caller.

```cpp
    Classification exceptionClassifier(std::exception_ptr e)
    {
        try
        {
            if (e)
                std::rethrow_exception(e);
            return Classification::Success;
        }
        catch (const MyTransientException&)
        {
            return Classification::Transient;
        }
        catch (...)
        {
            return Classification::Permanent;
        }
    }

    Classifier<HttpResponseCode> classifier = ClassifierBuilder<HttpResponseCode>()
        .withSuccessRange(100, 399)
        .withExceptionClassifier(&exceptionClassifier)
        .build();
```


# Retry callbacks
```RetryPP``` supports calling a callback before going to sleep when performing a retry. This is useful for updating UI or logging retries.
The callback is only called the operation experiences a transient failure. The callback receive an std::variant<T, std::exception_ptr> and
the calculated sleep interval before the next retry attempt. The std::exception_ptr is populated if the operation threw an exception that
was classified as transient.

```cpp
    void retryCallback(const std::variant<HttpResponseCode, std::exception_ptr>& result, std::chrono::milliseconds delay)
    {
        if (std::holds_alternative<HttpResponseCode>(result))
        {
            ... // do something with the response code fetchable by std::get<HttpResponseCode>(result)
        }
        else if (std::holds_alternative<std::exception_ptr>(result))
        {
            try
            {
                std::rethrow_exception(std::get<std::exception_ptr>(result));
            }
            catch (const MyTransientException& e)
            {
                ... // do something for MyTransientException
            }
            catch (const std::exception& e)
            {
                ... // do something for std::exception
            }
            catch (...)
            {
                ... // do something for unknown exceptions
            }
        }
    }

    Classifier<HttpResponseCode> classifier = ClassifierBuilder<HttpResponseCode>()
        .withSuccessRange(100, 299)
        .withRetryCallback(&retryCallback)
        .build();
```


# Async support (co_await)
...(to be documented)...

```cpp
    task<HttpResponseCode> asyncOperation()
    {
        ...
    }

    RetryResult<HttpResponseCode> result = co_await withAsyncRetry(policy, classifier, &asyncOperation);
```


# Classes


## RetryResult

```cpp
namespace RetryPP
{
    template<class Code>
    struct RetryResult
    {
        RetryResult(Classification classification, const Code& result) noexcept;

        RetryResult(const RetryResult&) noexcept = default;
        RetryResult(RetryResult&&) noexcept = default;
        RetryResult& operator=(const RetryResult&) noexcept = default;
        RetryResult& operator=(RetryResult&&) noexcept = default;
        ~RetryResult() = default;

        Classification classification;
        Code code;
    };
}
```


## Range
```cpp
namespace RetryPP
{
    template<class T, class Comp = std::less<T>>
    class Range
    {
    public:
        using Code = std::decay_t<T>;

        // Construct a Range with the specified start and end code (both inclusive).
        constexpr explicit Range(const Code& start, const Code& end) noexcept;

        constexpr Range(const Range&) noexcept = default;
        constexpr Range(Range&&) noexcept = default;
        constexpr Range& operator=(const Range&) noexcept = default;
        constexpr Range& operator=(Range&&) noexcept = default;
        ~Range() noexcept = default;

        // Return the start of the range.
        constexpr const Code& start() const noexcept;

        // Return the end of the range.
        constexpr const Code& end() const noexcept;

        // Returns true if specified code falls within the range.
        constexpr bool in_range(const Code& code) const noexcept;
    };
}
```


## Classification

```cpp
namespace RetryPP
{
    enum class Classification
    {
        Success,    // Indicates a successful reponse
        Transient,  // Indicates a transient (retryable) failure
        Permanent,  // Indicates a permanent (non-retyable) failure
    };
}
```


## PolicyBuilder

```cpp
namespace RetryPP
{
    class PolicyBuilder final : public internal::PolicyData
    {
    public:
        PolicyBuilder() noexcept = default;
        PolicyBuilder(const PolicyBuilder&) noexcept = default;
        PolicyBuilder(PolicyBuilder&&) noexcept = default;
        PolicyBuilder& operator=(const PolicyBuilder&) noexcept = default;
        PolicyBuilder& operator=(PolicyBuilder&&) noexcept = default;
        ~PolicyBuilder() noexcept = default;

        // Create a new PolicyBuilder with the settings of an existing Policy.
        explicit PolicyBuilder(const Policy& policy) noexcept;

        // Set the retry backoff strategy for the policy.
        template<RetryStrategy T, class... Args>
        PolicyBuilder& withStrategy(Args&&... args);

        // Add a Modifier to the policy.
        template<RetryBackoffModifier T, class... Args>
        PolicyBuilder& withModifier(Args&&... args);

        // Set the retry limit for the policy.
        template<RetryLimitPolicy T, class... Args>
        PolicyBuilder& withLimit(Args&&... args);

        // Clear the current list of modifiers.
        PolicyBuilder& clearModifiers() noexcept;

        // Build a new policy with the settings from the builder.
        // throws an InvalidPolicy exception if required options are missing.
        const Policy build() const;
    };
}
```

## ClassifierBuilder

```cpp
namespace RetryPP
{
    template<class T, class Comp = std::less<T>>
    class ClassifierBuilder final : public internal::ClassifierData<T, Comp>
    {
    public:
        using Code = internal::ClassifierData<T, Comp>::Code;
        using Range = internal::ClassifierData<T, Comp>::Range;

        ClassifierBuilder() noexcept = default;
        ClassifierBuilder(const ClassifierBuilder&) noexcept = default;
        ClassifierBuilder(ClassifierBuilder&&) noexcept = default;
        ClassifierBuilder& operator=(const ClassifierBuilder&) noexcept = default;
        ClassifierBuilder& operator=(ClassifierBuilder&&) noexcept = default;
        ~ClassifierBuilder() noexcept = default;

        // Create a new ClassifierBuilder with the settings of an existing classifier.
        explicit ClassifierBuilder(const Classifier<T, Comp>& classifier) noexcept;

        // Set codes and ranges indicating a successful operation.
        ClassifierBuilder& withSuccessCode(const Code& code);
        ClassifierBuilder& withSuccessCodes(const std::set<Code, Comp>& codes);
        ClassifierBuilder& withSuccessRange(const Code& start, const Code& end);

        // Set codes and ranges indicating a transient failure during operation.
        ClassifierBuilder& withTransientCode(const Code& code);
        ClassifierBuilder& withTransientCodes(const std::set<Code, Comp>& codes);
        ClassifierBuilder& withTransientRange(const Code& start, const Code& end);

        // Set codes and ranges indicating a permanent failure during operation.
        ClassifierBuilder& withPermanentCode(const Code& code);
        ClassifierBuilder& withPermanentCodes(const std::set<Code, Comp>& codes);
        ClassifierBuilder& withPermanentRange(const Code& start, const Code& end);

        // Set the classification for codes not defined as either successful, transient or permanent.
        ClassifierBuilder& withUndefinedCodeClassification(Classification classification) noexcept;

        // Set the callback for classifying exceptions.
        ClassifierBuilder& withExceptionClassifier(const std::function<Classification(std::exception_ptr)>& f);

        // Set the callback to be called before retrying an operation. The callback is called before sleeping.
        ClassifierBuilder& withRetryCallback(const std::function<void(const std::variant<Code, std::exception_ptr>&, std::chrono::milliseconds)>& f);

        // Build a classifier with the settings of the builder.
        const Classifier<Code, Comp> build() const;
    };
}
```


## Policy

```cpp
namespace RetryPP
{
    class Policy final : public internal::PolicyData
    {
    public:
        Policy(const Policy&) noexcept = default;
        Policy(Policy&&) noexcept = default;
        Policy& operator=(const Policy&) noexcept = default;
        Policy& operator=(Policy&&) noexcept = default;
        ~Policy() = default;

        // Returns an invalid policy.
        static Policy null() noexcept;

        // Returns true if policy is valid.
        bool valid() const noexcept;

        // Creates an instance of the policy's backoff strategy.
        std::unique_ptr<Strategy> createBackoffStrategy() const;

        // Creates an instance of the policy's limit policy.
        std::unique_ptr<Limit> createLimitPolicy() const;

        // Creates an std::list of the policy's modifiers.
        std::vector<std::unique_ptr<Modifier>> createBackoffModifiers() const;
    };
}
```


## Classifier

```cpp
namespace RetryPP
{
    template<class T, class Comp = std::less<T>>
    class Classifier final : public internal::ClassifierData<T, Comp>
    {
    public:
        using Code = internal::ClassifierData<T, Comp>::Code;

        Classifier(const Classifier&) noexcept = default;
        Classifier(Classifier&&) noexcept = default;
        Classifier& operator=(const Classifier&) noexcept = default;
        Classifier& operator=(Classifier&&) noexcept = default;
        ~Classifier() = default;

        // Create an invalid classifier.
        static Classifier null();

        // Returns true if classifier is valid.
        bool valid() const;

        // Returns success codes and ranges.
        const std::span<const Code> successCodes() const noexcept;
        const std::span<const Range> successRanges() const noexcept;

        // Returns transient codes and ranges.
        const std::span<const Code> transientCodes() const noexcept;
        const std::span<const Range> transientRanges() const noexcept;

        // Returns permanent codes and ranges.
        const std::span<const Code> permanentCodes() const noexcept;
        const std::span<const Range> permanentRanges() const noexcept;

        // Returns the exception classifier callback.
        const std::function<Classification(std::exception_ptr)> exceptionClassifier() const noexcept;

        // Functions to test a specified code
        bool isSuccessCode(const Code& code) const;
        bool isTransientCode(const Code& code) const;
        bool isPermanentCode(const Code& code) const;

        // Returns the classification of the supplied code.
        Classification classify(const Code& code) const;

        // Returns the classification of the supplied exception.
        Classification classify(std::exception_ptr e) const;

        // Calls the retry callback with the specified parameters.
        void onRetry(const std::variant<Code, std::exception_ptr>& result, std::chrono::milliseconds sleep) const;
    };
}
```

# Functions

## withRetry

```cpp
namespace RetryPP
{
    // Perform synchronous retry based on the supplied policy and classifier on the operation F&& f with (optional) arguments Args&&... args.
    // The stop_token allows aborting the retry sleep and returning immediately.
    template<class Code, class F, class... Args>
    RetryResult<Code> withRetry(const Policy& policy, const Classifier<Code>& classifier, std::stop_token stop_token, F&& f, Args&&... args);

    // Same as above except with no stop_token.
    template<class Code, class F, class... Args>
    RetryResult<Code> withRetry(const Policy& policy, const Classifier<Code>& classifier, F&& f, Args&&... args);
}
```


## withAsyncRetry

```cpp
namespace RetryPP
{
    // Perform asynchronous retry based on the supplied policy and classifier on the async operation F&& f with (optional) arguments Args&&... args.
    // The stop_token allows aborting the retry sleep and returning immediately.
    // The type of the resulting task is specified in the TaskResultType template parameter, other template parameters are auto-deduced if possible.
    template<class TaskResultType, class T, class F, class... Args>
    TaskResultType withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, std::stop_token stop_token, F&& f, Args&&... args);

    // Same as above except with no stop_token.
    template<class TaskResultType, class T, class F, class... Args>
    TaskResultType withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, F&& f, Args&&... args);

    // Template specializations of the above functions allowing auto-deducing the returned task type. Will work for task types with a single template parameter.
    template<class T, class F, class... Args>
    auto withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, std::stop_token stop_token, F&& f, Args&&... args) -> typename internal::wrapped_task<decltype(internal::function_return_type(f))>::type;

    template<class T, class F, class... Args>
    auto withAsyncRetry(const Policy& policy, const Classifier<T>& classifier, F&& f, Args&&... args) -> typename internal::wrapped_task<decltype(internal::function_return_type(f))>::type;
}
```


# License

### MIT License

##### Copyright (c) 2026 Thomas Barnekov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
