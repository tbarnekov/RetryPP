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


## Classifiers

The ```Classifier``` template is used to determine whether an operation should be retried or not. It is always used in combination
with a ```Policy``` and uses the result of an operation to classify it as a success, a temporary failure (retryable failure)
or a permanent (non-retryable) failure.

The type declared in the instantiation must match the return type of the operation being retried.
The type must satisfy the [TotallyOrdered](https://en.cppreference.com/w/cpp/concepts/totally_ordered.html) constraint meaning it
supports all comparison operators.

The ```Classifier``` allows you to specify either individual codes or ranges of codes for each category of success,
temporary failure or permanent failure.

Individual values have precedence over ranges. This means you can add a successful range and treat some codes as transient or
permanent even though they fall into the successful range or vice-versa.

RetryPP uses the Builder pattern to construct ```Classifier``` objects. Once created, the ```Classifier``` is immutable, thread-safe and
can be re-used as many times as you like.


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


## Build a ```Policy```
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


## Build a ```Classifier```


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
    HttpResponseCode result = withRetry(policy, classifier, &my_operation, code);

    ...
}
```

# Exception handling
(Pending documentation)


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
