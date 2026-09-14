//************************************************************
// callmebot.h - what a CallMeBot WhatsApp reply means
//
// painlessMesh reports what HTTP says: the status, whether a retry could
// deliver the request twice, and the start of the response body. Whether
// WhatsApp will actually show the message is CallMeBot's business, and
// CallMeBot does not put it in the status code:
//
//   * a rate-limit refusal comes back as HTTP 201 or HTTP 203 with an
//     "Oops! Too many requests" page (probed 2026-09-10, painlessMesh #450);
//   * HTTP 208 came back four times for messages that never arrived (#452);
//   * a paused account answers with the request echoed back and, at the end,
//     "Your Account is Paused ... send the word 'resume'" (#463);
//   * a message CallMeBot accepted answers "Message queued".
//
// So the sketch reads the reply with judge() below. It is plain C++ over the
// status and the body text, with no Arduino or network calls, so the desktop
// test suite checks it (test/catch/catch_callmebot_example.cpp) against the
// same CallMeBot-shaped test point the hardware farm uses.
//
// The phrases are CallMeBot's, observed or documented at the dates above. If
// CallMeBot changes its wording, a reply lands in Verdict::Unrecognized and
// the sketch prints what the service said -- it never guesses "sent".
//************************************************************
#ifndef SENDTOINTERNET_CALLMEBOT_H
#define SENDTOINTERNET_CALLMEBOT_H

#include "painlessmesh/gateway.hpp"

namespace callmebot {

enum class Verdict {
  /** CallMeBot accepted the message ("Message queued"). */
  Queued,
  /** "Too many requests": nothing was sent; wait before the next message. */
  RateLimited,
  /** The account is paused: send "resume" to the bot; resending won't help. */
  AccountPaused,
  /** HTTP 208: in the field this never meant a delivery, whatever the body. */
  NotDelivered,
  /** No HTTP reply at all; the library's error says why. */
  NoReply,
  /** A reply this file does not know. Treated as not sent. */
  Unrecognized
};

struct Judgement {
  Verdict verdict;
  /** True only for Verdict::Queued. */
  bool accepted;
  /**
   * Minimum wait before the sketch sends CallMeBot anything else, in ms.
   * Non-zero after a refusal, so a sketch does not turn one refusal into a
   * stream of them.
   */
  uint32_t holdOffMs;
  /** One line for the serial log. */
  const char* meaning;
};

/** How long to leave CallMeBot alone after it said "Too many requests". */
static const uint32_t RATE_LIMIT_HOLD_OFF_MS = 15UL * 60UL * 1000UL;

inline bool isHttpSuccess(uint16_t status) {
  return status == 200 || status == 201 || status == 202 || status == 204;
}

/**
 * Read a CallMeBot reply.
 *
 * @param httpStatus InternetResult::httpStatus (0 when no reply arrived)
 * @param response   InternetResult::response, the start of the body
 */
inline Judgement judge(uint16_t httpStatus, const TSTRING& response) {
  using painlessmesh::gateway::responseContains;

  if (httpStatus == 0) {
    return {Verdict::NoReply, false, 0,
            "no reply from CallMeBot; it may or may not have received the request"};
  }
  // Refusals first: they arrive under success statuses too.
  if (responseContains(response, "Too many requests")) {
    return {Verdict::RateLimited, false, RATE_LIMIT_HOLD_OFF_MS,
            "CallMeBot refused: too many requests"};
  }
  if (responseContains(response, "Account is Paused") ||
      responseContains(response, "send the word 'resume'")) {
    return {Verdict::AccountPaused, false, RATE_LIMIT_HOLD_OFF_MS,
            "CallMeBot account paused: send 'resume' to the CallMeBot bot on WhatsApp"};
  }
  // Before the success text: a 208 carrying "Message queued" still did not
  // arrive in the field.
  if (httpStatus == 208) {
    return {Verdict::NotDelivered, false, 0,
            "CallMeBot answered HTTP 208; such messages have not arrived"};
  }
  if (isHttpSuccess(httpStatus) && responseContains(response, "Message queued")) {
    return {Verdict::Queued, true, 0, "CallMeBot queued the message"};
  }
  return {Verdict::Unrecognized, false, 0,
          "CallMeBot's reply is not one this sketch recognises; not counted as sent"};
}

/**
 * The URL with the API key replaced, for printing. A serial log gets pasted
 * into issues; the key should not go with it.
 */
inline TSTRING redactApiKey(const TSTRING& url) {
#if defined(PAINLESSMESH_BOOST)
  const auto start = url.find("apikey=");
  if (start == TSTRING::npos) return url;
  const auto valueStart = start + 7;
  auto end = url.find('&', valueStart);
  if (end == TSTRING::npos) end = url.length();
  return url.substr(0, valueStart) + "***" + url.substr(end);
#else
  const int start = url.indexOf("apikey=");
  if (start < 0) return url;
  const int valueStart = start + 7;
  int end = url.indexOf('&', valueStart);
  if (end < 0) end = url.length();
  return url.substring(0, valueStart) + "***" + url.substring(end);
#endif
}

}  // namespace callmebot

#endif  // SENDTOINTERNET_CALLMEBOT_H
