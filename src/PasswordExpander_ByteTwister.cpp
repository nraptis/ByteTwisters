#include "PasswordExpander.hpp"

#include <cstdlib>

namespace peanutbutter::expansion::key_expansion {

namespace {

constexpr unsigned int kBlockLength =
    static_cast<unsigned int>(twist::PASSWORD_EXPANDED_SIZE);

}  // namespace

void PasswordExpander::Get(unsigned char* pSource,
                           unsigned char* pWorker,
                           unsigned char* pDestination,
                           unsigned int pLength) {
  if (mUseByteTwister) {
    if (mKeyBuffer != nullptr && mNextRoundKeyBuffer != nullptr) {
      ExpandPassword(mByteTwisterType, pSource, pWorker, pDestination, *mKeyBuffer, *mNextRoundKeyBuffer, pLength);
    } else {
      ExpandPassword(mByteTwisterType, pSource, pWorker, pDestination, pLength);
    }
    return;
  }
  if (mKeyBuffer == nullptr || mNextRoundKeyBuffer == nullptr) {
    std::abort();
  }
  ExpandPassword(mType, pSource, pWorker, pDestination, *mKeyBuffer, *mNextRoundKeyBuffer, pLength);
}

void PasswordExpander::ExpandPassword(ByteTwisterType pType,
                                      unsigned char* pSource,
                                      unsigned char* pWorker,
                                      unsigned char* pDestination,
                                      unsigned int pLength) {
  ExpandPasswordByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pLength);
}

void PasswordExpander::ExpandPassword(ByteTwisterType pType,
                                      unsigned char* pSource,
                                      unsigned char* pWorker,
                                      unsigned char* pDestination,
                                      unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                      unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                      unsigned int pLength) {
  ExpandPasswordByIndex(static_cast<unsigned char>(pType), pSource, pWorker, pDestination, pKeyBuffer, pNextRoundKeyBuffer, pLength);
}

void PasswordExpander::ExpandPasswordByIndex(unsigned char pType,
                                             unsigned char* pSource,
                                             unsigned char* pWorker,
                                             unsigned char* pDestination,
                                             unsigned int pLength) {
  if (pSource == nullptr || pDestination == nullptr) {
    std::abort();
  }
  if (pLength == 0U || (pLength % kBlockLength) != 0U) {
    std::abort();
  }
  ByteTwister::TwistBytesByIndex(pType, pSource, pWorker, pDestination, pLength);
}

void PasswordExpander::ExpandPasswordByIndex(unsigned char pType,
                                             unsigned char* pSource,
                                             unsigned char* pWorker,
                                             unsigned char* pDestination,
                                             unsigned char (&pKeyBuffer)[twist::kRoundKeyStackDepth][twist::kRoundKeyBytes],
                                             unsigned char (&pNextRoundKeyBuffer)[twist::kRoundKeyBytes],
                                             unsigned int pLength) {
  if (pSource == nullptr || pDestination == nullptr) {
    std::abort();
  }
  if (pLength == 0U || (pLength % kBlockLength) != 0U) {
    std::abort();
  }
  ByteTwister::TwistBytesByIndex(pType, pSource, pWorker, pDestination, pKeyBuffer, pNextRoundKeyBuffer, pLength);
}

}  // namespace peanutbutter::expansion::key_expansion
