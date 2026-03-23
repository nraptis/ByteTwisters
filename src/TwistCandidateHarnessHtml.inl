void WriteHtmlReport(
  const std::filesystem::path& path,
  const std::vector<CandidateResult>& results,
  const Options& options) {
  std::ofstream output(path);
  const std::vector<TrialSpec> trial_plan = BuildTrialPlan(options);
  std::size_t rejected = 0;
  for (const CandidateResult& result : results) {
    rejected += static_cast<std::size_t>(result.rejected);
  }

  std::ostringstream effective_config;
  effective_config
      << "input_mix = mixed\n"
      << "seed = " << options.seed << '\n'
      << "length_factor = " << options.length_factor << '\n'
      << "stream_bytes = " << options.stream_bytes
      << (options.stream_bytes_supplied ? " (override)" : " (derived from length_factor)") << '\n'
      << "stream_windows = " << WindowCountForBytes(options.stream_bytes) << '\n'
      << "trial_count = " << trial_plan.size() << '\n'
      << "sample_windows = " << options.sample_windows << '\n'
      << "signature_bytes = " << options.signature_bytes << '\n'
      << "avalanche_blocks = " << options.avalanche_blocks << '\n'
      << "avalanche_trials = " << options.avalanche_trials << '\n'
      << "bic_sample_bits = " << options.bic_sample_bits << '\n'
      << "second_order_trials = " << options.second_order_trials << '\n'
      << "cross_input_signature_bytes = " << options.cross_input_signature_bytes << '\n'
      << "long_repeat_scan_bytes = " << options.long_repeat_scan_bytes << '\n'
      << "long_repeat_windows = " << WindowCountForBytes(options.long_repeat_scan_bytes) << '\n'
      << "long_repeat_top_count = " << options.long_repeat_top_count << '\n'
      << "long_repeat_min_match_bytes = " << options.long_repeat_min_match_bytes << '\n'
      << "top_n = " << options.top_n << '\n'
      << "candidate_filter = "
      << (options.candidate_id_supplied ? std::to_string(options.candidate_id) : "all") << '\n'
      << "limit = " << (options.limit > 0U ? std::to_string(options.limit) : "none") << '\n'
      << "source_patterns = " << SourcePatternList(trial_plan);

  std::ostringstream knob_config;
  knob_config
      << "kRandomizeSeedByDefault = "
      << (knobs::kRandomizeSeedByDefault ? "true" : "false") << '\n'
      << "kRandomSeed = "
      << (knobs::kRandomSeed == 0U ? std::string("runtime-random") : std::to_string(knobs::kRandomSeed)) << '\n'
      << "kCandidateCount = " << knobs::kCandidateCount << '\n'
      << "kTopCandidateCount = " << knobs::kTopCandidateCount << '\n'
      << "kPhase1MinOps = " << knobs::kPhase1MinOps << '\n'
      << "kPhase1MaxOps = " << knobs::kPhase1MaxOps << '\n'
      << "kPhase2MinOps = " << knobs::kPhase2MinOps << '\n'
      << "kPhase2MaxOps = " << knobs::kPhase2MaxOps << '\n'
      << "kMaxTransformsTotal = " << FormatLimit(knobs::kMaxTransformsTotal) << '\n'
      << "kMaxAddOps = " << FormatLimit(knobs::kMaxAddOps) << '\n'
      << "kMaxSubOps = " << FormatLimit(knobs::kMaxSubOps) << '\n'
      << "kMaxMulOps = " << FormatLimit(knobs::kMaxMulOps) << '\n'
      << "kMaxXorOps = " << FormatLimit(knobs::kMaxXorOps) << '\n'
      << "kMaxAndOps = " << FormatLimit(knobs::kMaxAndOps) << '\n'
      << "kMaxOrOps = " << FormatLimit(knobs::kMaxOrOps) << '\n'
      << "kMaxAddConstTransforms = " << FormatLimit(knobs::kMaxAddConstTransforms) << '\n'
      << "kMaxShiftLeftTransforms = " << FormatLimit(knobs::kMaxShiftLeftTransforms) << '\n'
      << "kMaxShiftRightTransforms = " << FormatLimit(knobs::kMaxShiftRightTransforms) << '\n'
      << "kMaxNotTransforms = " << FormatLimit(knobs::kMaxNotTransforms) << '\n'
      << "kMaxSwapNibblesTransforms = " << FormatLimit(knobs::kMaxSwapNibblesTransforms) << '\n'
      << "kMaxByteLR8LeftTransforms = " << FormatLimit(knobs::kMaxByteLR8LeftTransforms) << '\n'
      << "kMaxByteLR8RightTransforms = " << FormatLimit(knobs::kMaxByteLR8RightTransforms) << '\n'
      << "kLengthFactor = " << knobs::kLengthFactor << '\n'
      << "kTrialCountAES = " << knobs::kTrialCountAES << '\n'
      << "kTrialCountChaCha = " << knobs::kTrialCountChaCha << '\n'
      << "kTrialCountZeros = " << knobs::kTrialCountZeros << '\n'
      << "kTrialCountOnes = " << knobs::kTrialCountOnes << '\n'
      << "kTrialCountPredictableA = " << knobs::kTrialCountPredictableA << '\n'
      << "kTrialCountPredictableB = " << knobs::kTrialCountPredictableB << '\n'
      << "kTrialCountPredictableC = " << knobs::kTrialCountPredictableC << '\n'
      << "kDefaultSampleWindows = " << knobs::kDefaultSampleWindows << '\n'
      << "kDefaultSignatureBytes = " << knobs::kDefaultSignatureBytes << '\n'
      << "kDefaultAvalancheBlocks = " << knobs::kDefaultAvalancheBlocks << '\n'
      << "kDefaultAvalancheTrials = " << knobs::kDefaultAvalancheTrials << '\n'
      << "kDefaultBicSampleBits = " << knobs::kDefaultBicSampleBits << '\n'
      << "kDefaultSecondOrderTrials = " << knobs::kDefaultSecondOrderTrials << '\n'
      << "kDefaultCrossInputSignatureBytes = " << knobs::kDefaultCrossInputSignatureBytes << '\n'
      << "kLongRepeatScanBytes = " << knobs::kLongRepeatScanBytes << '\n'
      << "kLongRepeatTopCandidateCount = " << knobs::kLongRepeatTopCandidateCount << '\n'
      << "kLongRepeatMinMatchBytes = " << knobs::kLongRepeatMinMatchBytes << '\n'
      << "kEnableThreeMatrix = " << (knobs::kEnableThreeMatrix ? "true" : "false") << '\n'
      << "kThreeMatrixRatioPercent = " << knobs::kThreeMatrixRatioPercent;

  output << "<!doctype html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "  <meta charset=\"utf-8\">\n"
         << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
         << "  <title>Twist Candidate Report</title>\n"
         << "  <style>\n"
         << "    :root { color-scheme: light; }\n"
         << "    body { margin: 0; font: 14px/1.5 Menlo, Monaco, 'SFMono-Regular', monospace; background: #f4efe5; color: #1f1a17; }\n"
         << "    main { max-width: 1200px; margin: 0 auto; padding: 32px 24px 48px; }\n"
         << "    h1, h2 { margin: 0 0 12px; }\n"
         << "    h1 { font-size: 28px; }\n"
         << "    h2 { font-size: 18px; margin-top: 28px; }\n"
         << "    .meta { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 12px; margin: 20px 0 28px; }\n"
         << "    .config-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(360px, 1fr)); gap: 12px; margin: 18px 0 28px; }\n"
         << "    .card { background: #fffaf0; border: 1px solid #d8c9b2; border-radius: 12px; padding: 12px 14px; }\n"
         << "    .label { color: #7b5a3a; font-size: 12px; text-transform: uppercase; letter-spacing: 0.08em; }\n"
         << "    .value { font-size: 20px; margin-top: 4px; }\n"
         << "    .method { background: #fffaf0; border-left: 4px solid #ba8f56; padding: 14px 16px; border-radius: 8px; }\n"
         << "    .config-card pre { margin: 10px 0 0; white-space: pre-wrap; word-break: break-word; }\n"
         << "    table { width: 100%; border-collapse: collapse; margin-top: 14px; background: #fffaf0; border: 1px solid #d8c9b2; }\n"
         << "    th, td { padding: 10px 8px; border-bottom: 1px solid #eadcc6; vertical-align: top; text-align: left; }\n"
         << "    th { position: sticky; top: 0; background: #f2e3cc; }\n"
         << "    tr:nth-child(even) td { background: #fffcf7; }\n"
         << "    .grade { font-weight: 700; }\n"
         << "  </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "<main>\n"
         << "  <h1>Twist Candidate Report</h1>\n"
         << "  <p>Sorted by grade, then composite score.</p>\n"
         << "  <section class=\"meta\">\n"
         << "    <div class=\"card\"><div class=\"label\">Blocks</div><div class=\"value\">" << options.length_factor << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Trials</div><div class=\"value\">" << trial_plan.size() << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Input Mix</div><div class=\"value\">mixed</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Seed</div><div class=\"value\">" << options.seed << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Candidates</div><div class=\"value\">" << results.size() << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Rejected</div><div class=\"value\">" << rejected << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Stream Bytes</div><div class=\"value\">" << options.stream_bytes << "</div></div>\n"
         << "    <div class=\"card\"><div class=\"label\">Long Repeat Bytes</div><div class=\"value\">" << options.long_repeat_scan_bytes << "</div></div>\n"
         << "  </section>\n"
         << "  <section class=\"method\">\n"
         << "    <strong>Methodology.</strong> Each trial starts from the configured category mix shown below, then the harness measures how well each twister converts that input into high-quality output under repeated feedback. "
         << "Population scoring uses sampled rolling 128-bit windows, exact block-cycle detection, whole-window diffusion checkpoints, BIC sampling, second-order differential checks, cross-input signature separation, and a long-repeat scan for repeated substrings above the configured minimum match length. "
         << "The sections below show the effective runtime settings and the compiled defaults from Knobs.hpp.\n"
         << "  </section>\n"
         << "  <section class=\"config-grid\">\n"
         << "    <div class=\"card config-card\">\n"
         << "      <div class=\"label\">Effective Run Config</div>\n"
         << "      <pre>" << EscapeHtml(effective_config.str()) << "</pre>\n"
         << "    </div>\n"
         << "    <div class=\"card config-card\">\n"
         << "      <div class=\"label\">Knobs.hpp Defaults</div>\n"
         << "      <pre>" << EscapeHtml(knob_config.str()) << "</pre>\n"
         << "    </div>\n"
         << "  </section>\n";

  const std::vector<std::string> grade_order = {
      "A+", "A", "A-", "B+", "B", "B-", "C+", "C", "C-", "D+", "D", "D-", "F"};

  for (const std::string& grade : grade_order) {
    bool wrote_header = false;
    output << "  <section>\n";
    for (const CandidateResult& result : results) {
      if (result.grade != grade) {
        continue;
      }
      if (!wrote_header) {
        output << "    <h2>" << grade << "</h2>\n"
               << "    <table>\n"
               << "      <thead><tr>"
               << "<th>Rank</th><th>Candidate</th><th>Composite</th><th>Repeat</th><th>Cycle</th>"
               << "<th>Uniformity</th><th>Spread</th><th>Most</th><th>Least</th><th>Gap</th>"
               << "<th>Predictability</th><th>Avalanche</th><th>Complete</th><th>BIC</th><th>Inclusion</th><th>Nonlinear</th><th>XInput</th><th>Distinctness</th>"
               << "<th>Repeat Scan</th><th>Rejected</th><th>Failure</th>"
               << "</tr></thead>\n"
               << "      <tbody>\n";
        wrote_header = true;
      }
    }

    if (!wrote_header) {
      output << "  </section>\n";
      continue;
    }

    std::size_t rank = 0;
    for (const CandidateResult& result : results) {
      if (result.grade != grade) {
        continue;
      }
      rank += 1U;
      output << "        <tr>"
             << "<td>" << rank << "</td>"
             << "<td class=\"grade\">" << result.candidate_id << " / "
             << EscapeHtml(result.function_name) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.composite_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.repeat_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.cycle_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.uniformity_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.byte_count_spread_ratio)) << "</td>"
             << "<td>" << result.most_common_byte_count << "</td>"
             << "<td>" << result.least_common_byte_count << "</td>"
             << "<td>" << (result.most_common_byte_count - result.least_common_byte_count) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.predictability_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.avalanche_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.completeness_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.bic_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.bit_inclusion_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.nonlinearity_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.cross_input_collision_score)) << "</td>"
             << "<td>" << EscapeHtml(FormatDouble(result.distinctness_score)) << "</td>"
             << "<td>"
             << (result.long_repeat_match_found
                     ? (std::string("match@") + std::to_string(result.long_repeat_match_position) +
                        " len=" + std::to_string(result.long_repeat_match_length) +
                        " / " + LabelForGlobalTrialIndex(result.long_repeat_match_trial))
                     : (result.long_repeat_verified ? "clear" : "not-run"))
             << "</td>"
             << "<td>" << (result.rejected ? "true" : "false") << "</td>"
             << "<td>" << EscapeHtml(result.failure_reason) << "</td>"
             << "</tr>\n";
    }

    output << "      </tbody>\n"
           << "    </table>\n"
           << "  </section>\n";
  }

  output << "</main>\n"
         << "</body>\n"
         << "</html>\n";
}
