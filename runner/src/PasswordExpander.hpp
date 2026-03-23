#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_HPP_

#include <cstddef>
#include <cstdint>

#include "ByteTwister.hpp"
#include "Scrambler.hpp"
#include "TwistTypes.hpp"

namespace peanutbutter::expansion::key_expansion {

class PasswordExpander final : public peanutbutter::rng::Scrambler {
 public:
  using Type = twist::TwistFunction;
  using ByteTwisterType = ByteTwister::Type;
  static constexpr ByteTwisterType kType00 = ByteTwister::kType00;
  static constexpr ByteTwisterType kType01 = ByteTwister::kType01;
  static constexpr ByteTwisterType kType02 = ByteTwister::kType02;
  static constexpr ByteTwisterType kType03 = ByteTwister::kType03;
  static constexpr ByteTwisterType kType04 = ByteTwister::kType04;
  static constexpr ByteTwisterType kType05 = ByteTwister::kType05;
  static constexpr ByteTwisterType kType06 = ByteTwister::kType06;
  static constexpr ByteTwisterType kType07 = ByteTwister::kType07;
  static constexpr ByteTwisterType kType08 = ByteTwister::kType08;
  static constexpr ByteTwisterType kType09 = ByteTwister::kType09;
  static constexpr ByteTwisterType kType10 = ByteTwister::kType10;
  static constexpr ByteTwisterType kType11 = ByteTwister::kType11;
  static constexpr ByteTwisterType kType12 = ByteTwister::kType12;
  static constexpr ByteTwisterType kType13 = ByteTwister::kType13;
  static constexpr ByteTwisterType kType14 = ByteTwister::kType14;
  static constexpr ByteTwisterType kType15 = ByteTwister::kType15;
  static constexpr int kTypeCount = ByteTwister::kTypeCount;

  explicit PasswordExpander(Type pType = nullptr)
      : mType(pType),
        mByteTwisterType(ByteTwister::kType00),
        mUseByteTwister(false),
        mKeyBuffer(nullptr),
        mMaskBufferA(nullptr),
        mMaskBufferB(nullptr),
        mNextRoundKeyBuffer(nullptr),
        mNextRoundMaskBufferA(nullptr),
        mNextRoundMaskBufferB(nullptr) {}

  explicit PasswordExpander(ByteTwisterType pType)
      : mType(nullptr),
        mByteTwisterType(pType),
        mUseByteTwister(true),
        mKeyBuffer(nullptr),
        mMaskBufferA(nullptr),
        mMaskBufferB(nullptr),
        mNextRoundKeyBuffer(nullptr),
        mNextRoundMaskBufferA(nullptr),
        mNextRoundMaskBufferB(nullptr) {}

  void SetType(Type pType) { mType = pType; mUseByteTwister = false; }
  Type CurrentType() const { return mType; }
  void SetByteTwisterType(ByteTwisterType pType) { mByteTwisterType = pType; mUseByteTwister = true; }
  ByteTwisterType CurrentByteTwisterType() const { return mByteTwisterType; }
  bool UsesByteTwister() const { return mUseByteTwister; }
  void SetKeyBuffer(unsigned char (*pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes]) { mKeyBuffer = pKeyBuffer; }
  unsigned char (*CurrentKeyBuffer() const)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes] { return mKeyBuffer; }
  void SetMaskBufferA(unsigned char (*pMaskBuffer)[twist::kMaskStackDepth][twist::kMaskBytes]) { mMaskBufferA = pMaskBuffer; }
  unsigned char (*CurrentMaskBufferA() const)[twist::kMaskStackDepth][twist::kMaskBytes] { return mMaskBufferA; }
  void SetMaskBufferB(unsigned char (*pMaskBuffer)[twist::kMaskStackDepth][twist::kMaskBytes]) { mMaskBufferB = pMaskBuffer; }
  unsigned char (*CurrentMaskBufferB() const)[twist::kMaskStackDepth][twist::kMaskBytes] { return mMaskBufferB; }
  void SetNextRoundKeyBuffer(unsigned char (*pNextRoundKeyBuffer)[twist::kRoundKeyBytes]) { mNextRoundKeyBuffer = pNextRoundKeyBuffer; }
  unsigned char (*CurrentNextRoundKeyBuffer() const)[twist::kRoundKeyBytes] { return mNextRoundKeyBuffer; }
  void SetNextRoundMaskBufferA(unsigned char (*pNextRoundMaskBuffer)[twist::kMaskBytes]) { mNextRoundMaskBufferA = pNextRoundMaskBuffer; }
  unsigned char (*CurrentNextRoundMaskBufferA() const)[twist::kMaskBytes] { return mNextRoundMaskBufferA; }
  void SetNextRoundMaskBufferB(unsigned char (*pNextRoundMaskBuffer)[twist::kMaskBytes]) { mNextRoundMaskBufferB = pNextRoundMaskBuffer; }
  unsigned char (*CurrentNextRoundMaskBufferB() const)[twist::kMaskBytes] { return mNextRoundMaskBufferB; }

  void Get(unsigned char* pSource,
           unsigned char* pWorker,
           unsigned char* pDestination,
           unsigned int pLength) override;

  static void FillDoubledSource(const unsigned char* pPassword,
                                unsigned int pPasswordLength,
                                unsigned char* pSourceBuffer);
  static void SeedRegisteredCandidate(const twist::RegisteredCandidate& pCandidate,
                                      unsigned char* pSource,
                                      unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                      unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                      unsigned char (&pMaskBufferA)[twist::kMaskStackDepth][twist::kMaskBytes],
                                      unsigned char (&pMaskBufferB)[twist::kMaskStackDepth][twist::kMaskBytes],
                                      unsigned int pLength);
  static void TwistRegisteredCandidateBlock(const twist::RegisteredCandidate& pCandidate,
                                            unsigned char* pSource,
                                            unsigned char* pWorkerA,
                                            unsigned char* pWorkerB,
                                            unsigned char* pDestination,
                                            unsigned int pRound,
                                            const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                            unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                            unsigned char (&pMaskBufferA)[twist::kMaskStackDepth][twist::kMaskBytes],
                                            unsigned char (&pMaskBufferB)[twist::kMaskStackDepth][twist::kMaskBytes],
                                            unsigned int pLength);
  static void PushRegisteredCandidateRound(const twist::RegisteredCandidate& pCandidate,
                                           unsigned char* pDestination,
                                           const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                           unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                           unsigned char (&pMaskBufferA)[twist::kMaskStackDepth][twist::kMaskBytes],
                                           unsigned char (&pMaskBufferB)[twist::kMaskStackDepth][twist::kMaskBytes],
                                           unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                           unsigned char (&pNextRoundMaskBufferA)[twist::kMaskBytes],
                                           unsigned char (&pNextRoundMaskBufferB)[twist::kMaskBytes],
                                           unsigned int pLength);
  static void ExpandPassword(const twist::RegisteredCandidate& pCandidate,
                             unsigned char* pSource,
                             unsigned char* pWorkerA,
                             unsigned char* pWorkerB,
                             unsigned char* pDestination,
                             unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                             unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                             unsigned char (&pMaskBufferA)[twist::kMaskStackDepth][twist::kMaskBytes],
                             unsigned char (&pMaskBufferB)[twist::kMaskStackDepth][twist::kMaskBytes],
                             unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                             unsigned char (&pNextRoundMaskBufferA)[twist::kMaskBytes],
                             unsigned char (&pNextRoundMaskBufferB)[twist::kMaskBytes],
                             unsigned int pLength);
  static void ExpandPassword(Type pType,
                             unsigned char* pSource,
                             unsigned char* pWorkerA,
                             unsigned char* pWorkerB,
                             unsigned char* pDestination,
                             unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                             unsigned char (&pMaskBufferA)[twist::kMaskStackDepth][twist::kMaskBytes],
                             unsigned char (&pMaskBufferB)[twist::kMaskStackDepth][twist::kMaskBytes],
                             unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                             unsigned char (&pNextRoundMaskBufferA)[twist::kMaskBytes],
                             unsigned char (&pNextRoundMaskBufferB)[twist::kMaskBytes],
                             unsigned int pLength);
  // ByteTwister owns blocking across the full output length. PasswordExpander must pass the
  // total destination length here and must not pre-split into PASSWORD_EXPANDED_SIZE blocks.
  // If the parent project wants to change twisters block-by-block, it should call the
  // ByteTwister seed/block/push hooks directly instead of this wrapper.
  static void ExpandPassword(ByteTwisterType pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned int pLength);
  static void ExpandPassword(ByteTwisterType pType,
                             unsigned char* pSource,
                             unsigned char* pWorker,
                             unsigned char* pDestination,
                             unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                             unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                             unsigned int pLength);
  static void ExpandPasswordByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned int pLength);
  static void ExpandPasswordByIndex(unsigned char pType,
                                    unsigned char* pSource,
                                    unsigned char* pWorker,
                                    unsigned char* pDestination,
                                    unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                    unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                    unsigned int pLength);

 private:
  Type mType;
  ByteTwisterType mByteTwisterType;
  bool mUseByteTwister;
  unsigned char (*mKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes];
  unsigned char (*mMaskBufferA)[twist::kMaskStackDepth][twist::kMaskBytes];
  unsigned char (*mMaskBufferB)[twist::kMaskStackDepth][twist::kMaskBytes];
  unsigned char (*mNextRoundKeyBuffer)[twist::kRoundKeyBytes];
  unsigned char (*mNextRoundMaskBufferA)[twist::kMaskBytes];
  unsigned char (*mNextRoundMaskBufferB)[twist::kMaskBytes];
};

}  // namespace peanutbutter::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_PASSWORD_EXPANDER_HPP_
