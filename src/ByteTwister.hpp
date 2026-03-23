#ifndef BREAD_SRC_EXPANSION_KEY_EXPANSION_BYTE_TWISTER_HPP_
#define BREAD_SRC_EXPANSION_KEY_EXPANSION_BYTE_TWISTER_HPP_

#include <cstddef>

#include "Scrambler.hpp"
#include "TwistTypes.hpp"

namespace peanutbutter::expansion::key_expansion {

class ByteTwister final : public peanutbutter::rng::Scrambler {
 public:
  enum Type {
    kType00 = 0,
    kType01,
    kType02,
    kType03,
    kType04,
    kType05,
    kType06,
    kType07,
    kType08,
    kType09,
    kType10,
    kType11,
    kType12,
    kType13,
    kType14,
    kType15,
    kTypeCount
  };

  static constexpr int kBufferLength = static_cast<int>(twist::PASSWORD_EXPANDED_SIZE);

  explicit ByteTwister(Type pType = kType00)
      : mType(pType) {}

  void SetType(Type pType) { mType = pType; }
  Type CurrentType() const { return mType; }

  void Get(unsigned char* pSource,
           unsigned char* pWorker,
           unsigned char* pDestination,
           unsigned int pLength) override;

  // Hook-style entry points for callers that want to seed once and schedule
  // per-block twister changes themselves.
  static void SeedKey(Type pType,
                      unsigned char* pSource,
                      unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                      unsigned int pLength);
  static void SeedKeyByIndex(unsigned char pType,
                             unsigned char* pSource,
                             unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                             unsigned int pLength);

  static void SeedSalt(Type pType,
                       unsigned char* pSource,
                       unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                       unsigned int pLength);
  static void SeedSaltByIndex(unsigned char pType,
                              unsigned char* pSource,
                              unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                              unsigned int pLength);

  static void TwistBlock(Type pType,
                         unsigned char* pSource,
                         unsigned char* pWorker,
                         unsigned char* pDestination,
                         unsigned int pRound,
                         const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                         unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                         unsigned int pLength);
  static void TwistBlockByIndex(unsigned char pType,
                                unsigned char* pSource,
                                unsigned char* pWorker,
                                unsigned char* pDestination,
                                unsigned int pRound,
                                const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                unsigned int pLength);

  static void PushKeyRound(Type pType,
                           unsigned char* pDestination,
                           const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                           unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                           unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                           unsigned int pLength);
  static void PushKeyRoundByIndex(unsigned char pType,
                                  unsigned char* pDestination,
                                  const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
                                  unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                  unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                  unsigned int pLength);

  static void TwistBytes(Type pType,
                         unsigned char* pSource,
                         unsigned char* pWorker,
                         unsigned char* pDestination,
                         unsigned int pLength);
  static void TwistBytes(Type pType,
                         unsigned char* pSource,
                         unsigned char* pWorker,
                         unsigned char* pDestination,
                         unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                         unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                         unsigned int pLength);
  static void TwistBytesByIndex(unsigned char pType,
                                unsigned char* pSource,
                                unsigned char* pWorker,
                                unsigned char* pDestination,
                                unsigned int pLength);
  static void TwistBytesByIndex(unsigned char pType,
                                unsigned char* pSource,
                                unsigned char* pWorker,
                                unsigned char* pDestination,
                                unsigned char (&pKeyStack)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                unsigned int pLength);

 private:
  Type mType;
};

}  // namespace peanutbutter::expansion::key_expansion

#endif  // BREAD_SRC_EXPANSION_KEY_EXPANSION_BYTE_TWISTER_HPP_
