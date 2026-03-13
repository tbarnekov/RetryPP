# Changelog for RetryPP

## 1.0.1 (March 13, 2026)

### New

- Add unit tests

### Bugfixes

#### General

- Fix missing includes (primarily Exceptions.h)
- Fix linker errors due to missing inline keywords

#### Range class

- Range was not copy assignable/movable due to const member variables

#### Exponential class

- Renamed scaling() member to multiplier() to match the parameter name in constructor
- Fix that multiplier was accidentally cast to integer when performing next delay calculation

#### Immediate class

- Add missing Immediate constructor

#### Jitter algorithm classes

- Change random implementations to return better randomized results

#### Classifier class

- The valid() member function returned wrong results


## 1.0.0 (March 10, 2026)

Initial release
