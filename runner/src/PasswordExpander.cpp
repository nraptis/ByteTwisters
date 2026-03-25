#include "PasswordExpander.hpp"

#include <cstdlib>
#include <cstring>

namespace peanutbutter::expansion::key_expansion {

namespace {

constexpr unsigned int kBlockLength =
    static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE);

void FillRepeatedSource(const unsigned char* pPassword,
                        unsigned int pPasswordLength,
                        unsigned int pDestinationLength,
                        unsigned char* pSourceBuffer) {
  if (pSourceBuffer == nullptr) {
    return;
  }
  if (pPassword == nullptr || pPasswordLength == 0U) {
    std::memset(pSourceBuffer, 0, pDestinationLength);
    return;
  }

  const unsigned int aInitialCopy =
      (pPasswordLength < pDestinationLength) ? pPasswordLength : pDestinationLength;
  std::memcpy(pSourceBuffer, pPassword, static_cast<std::size_t>(aInitialCopy));
  unsigned int aFilled = aInitialCopy;
  while (aFilled < pDestinationLength) {
    const unsigned int aRemaining = pDestinationLength - aFilled;
    const unsigned int aChunk = (aRemaining < aFilled) ? aRemaining : aFilled;
    std::memcpy(pSourceBuffer + aFilled, pSourceBuffer, static_cast<std::size_t>(aChunk));
    aFilled += aChunk;
  }
}

}  // namespace

void PasswordExpander::FillDoubledSource(const unsigned char* pPassword,
                                         unsigned int pPasswordLength,
                                         unsigned char* pSourceBuffer) {
  FillRepeatedSource(pPassword, pPasswordLength, kBlockLength, pSourceBuffer);
}

void PasswordExpander::SeedRegisteredCandidate(
    const twist::RegisteredCandidate& pCandidate,
    unsigned char* pSource,
    unsigned char (&pSaltBuffer)[twist::kSaltBytes],
    unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
    unsigned char (&pMaskBufferA)[twist::kMaskStackDepth][twist::kMaskBytes],
    unsigned char (&pMaskBufferB)[twist::kMaskStackDepth][twist::kMaskBytes],
    unsigned int pLength) {
  if (pSource == nullptr || pLength < kBlockLength) {
    std::abort();
  }
  std::memset(pSaltBuffer, 0, twist::kSaltBytes);
  std::memset(pKeyBuffer, 0, twist::kRoundKeyStackDepth * twist::kRoundKeyBytes);
  std::memset(pMaskBufferA, 0, twist::kMaskStackDepth * twist::kMaskBytes);
  std::memset(pMaskBufferB, 0, twist::kMaskStackDepth * twist::kMaskBytes);
  if (pCandidate.key_seed != nullptr) {
    pCandidate.key_seed(pSource, pKeyBuffer, kBlockLength);
  }
  if (pCandidate.mask_seed_a != nullptr) {
    pCandidate.mask_seed_a(pSource, pMaskBufferA, kBlockLength);
  }
  if (pCandidate.mask_seed_b != nullptr) {
    pCandidate.mask_seed_b(pSource, pMaskBufferB, kBlockLength);
  }
  if (pCandidate.salt_seed != nullptr) {
    pCandidate.salt_seed(pSource, pSaltBuffer, kBlockLength);
  }
}

void PasswordExpander::TwistRegisteredCandidateBlock(
    const twist::RegisteredCandidate& pCandidate,
    unsigned char* pSource,
    unsigned char* pWorkerA,
    unsigned char* pWorkerB,
    unsigned char* pDestination,
    unsigned int pRound,
    const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
    unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
    unsigned char (&pMaskBufferA)[twist::kMaskStackDepth][twist::kMaskBytes],
    unsigned char (&pMaskBufferB)[twist::kMaskStackDepth][twist::kMaskBytes],
    unsigned int pLength) {
  if (pDestination == nullptr || pLength < kBlockLength) {
    std::abort();
  }
  if (pCandidate.twist_block != nullptr) {
    unsigned char aBreakerTempA[twist::kMatrixBlockBytes]{};
    unsigned char aBreakerTempB[twist::kMatrixBlockBytes]{};
    pCandidate.twist_block(
        pSource,
        pWorkerA,
        pWorkerB,
        pDestination,
        aBreakerTempA,
        aBreakerTempB,
        pRound,
        pSaltBuffer,
        pKeyBuffer,
        pMaskBufferA,
        pMaskBufferB,
        kBlockLength);
    return;
  }
  if (pCandidate.function == nullptr) {
    std::abort();
  }
  unsigned char aNextRoundKey[twist::kRoundKeyBytes]{};
  unsigned char aNextRoundMaskA[twist::kMaskBytes]{};
  unsigned char aNextRoundMaskB[twist::kMaskBytes]{};
  pCandidate.function(
      pSource,
      pWorkerA,
      pWorkerB,
      pDestination,
      pKeyBuffer,
      pMaskBufferA,
      pMaskBufferB,
      aNextRoundKey,
      aNextRoundMaskA,
      aNextRoundMaskB,
      kBlockLength);
}

void PasswordExpander::PushRegisteredCandidateRound(
    const twist::RegisteredCandidate& pCandidate,
    unsigned char* pDestination,
    const unsigned char (&pSaltBuffer)[twist::kSaltBytes],
    unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
    unsigned char (&pMaskBufferA)[twist::kMaskStackDepth][twist::kMaskBytes],
    unsigned char (&pMaskBufferB)[twist::kMaskStackDepth][twist::kMaskBytes],
    unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
    unsigned char (&pNextRoundMaskBufferA)[twist::kMaskBytes],
    unsigned char (&pNextRoundMaskBufferB)[twist::kMaskBytes],
    unsigned int pLength) {
  if (pDestination == nullptr || pLength < kBlockLength) {
    std::abort();
  }
  if (pCandidate.push_key_round != nullptr) {
    pCandidate.push_key_round(
        pDestination,
        pSaltBuffer,
        pKeyBuffer,
        pNextRoundKeyBuffer,
        kBlockLength);
  }
  if (pCandidate.push_mask_round_a != nullptr) {
    pCandidate.push_mask_round_a(
        pDestination,
        pMaskBufferA,
        pMaskBufferB,
        pNextRoundMaskBufferA,
        kBlockLength);
  }
  if (pCandidate.push_mask_round_b != nullptr) {
    pCandidate.push_mask_round_b(
        pDestination,
        pMaskBufferB,
        pMaskBufferA,
        pNextRoundMaskBufferB,
        kBlockLength);
  }
}

void PasswordExpander::ExpandPassword(
    const twist::RegisteredCandidate& pCandidate,
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
    unsigned int pLength) {
  if (pSource == nullptr || pDestination == nullptr) {
    std::abort();
  }
  if (pLength == 0U || (pLength % kBlockLength) != 0U) {
    std::abort();
  }

  SeedRegisteredCandidate(pCandidate, pSource, pSaltBuffer, pKeyBuffer, pMaskBufferA, pMaskBufferB, kBlockLength);

  unsigned int aRound = 0U;
  for (unsigned int aOffset = 0U; aOffset < pLength; aOffset += kBlockLength, ++aRound) {
    unsigned char* aRoundSource = (aRound == 0U) ? pSource : (pDestination + aOffset - kBlockLength);
    unsigned char* aRoundDest = pDestination + aOffset;
    TwistRegisteredCandidateBlock(
        pCandidate,
        aRoundSource,
        pWorkerA,
        pWorkerB,
        aRoundDest,
        aRound,
        pSaltBuffer,
        pKeyBuffer,
        pMaskBufferA,
        pMaskBufferB,
        kBlockLength);
    PushRegisteredCandidateRound(
        pCandidate,
        aRoundDest,
        pSaltBuffer,
        pKeyBuffer,
        pMaskBufferA,
        pMaskBufferB,
        pNextRoundKeyBuffer,
        pNextRoundMaskBufferA,
        pNextRoundMaskBufferB,
        kBlockLength);
  }
}

void PasswordExpander::ExpandPassword(Type pType,
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
                                      unsigned int pLength) {
  if (pType == nullptr || pSource == nullptr || pDestination == nullptr) {
    std::abort();
  }
  if (pLength == 0U || (pLength % kBlockLength) != 0U) {
    std::abort();
  }
  pType(
      pSource,
      pWorkerA,
      pWorkerB,
      pDestination,
      pKeyBuffer,
      pMaskBufferA,
      pMaskBufferB,
      pNextRoundKeyBuffer,
      pNextRoundMaskBufferA,
      pNextRoundMaskBufferB,
      pLength);
}

}  // namespace peanutbutter::expansion::key_expansion
