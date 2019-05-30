/*
* Author: Rongyang Sun <sun-rongyang@outlook.com>
* Creation Date: 2019-05-14 12:22
* 
* Description: GraceQ/mps2 project. Private objects for two sites algorithm. Implementation.
*/
#include "two_site_algo.h"
#include "gqmps2/gqmps2.h"
#include "gqten/gqten.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#ifdef Release
  #define NDEBUG
#endif
#include <assert.h>


namespace gqmps2 {
using namespace gqten;


std::pair<std::vector<GQTensor *>, std::vector<GQTensor *>> InitBlocks(
    const std::vector<GQTensor *> &mps, const std::vector<GQTensor *> &mpo,
    const SweepParams &sweep_params) {
  assert(mps.size() == mpo.size());
  auto N = mps.size();
  std::vector<GQTensor *> rblocks(N-1);
  std::vector<GQTensor *> lblocks(N-1);

  if (sweep_params.Workflow == kTwoSiteAlgoWorkflowContinue) {
    return std::make_pair(lblocks, rblocks);
  }

  // Generate blocks.
  auto rblock0 = new GQTensor();
  rblocks[0] = rblock0;
  auto rblock1 = Contract(*mps.back(), *mpo.back(), {{1}, {0}});
  auto temp_rblock1 = Contract(*rblock1, MockDag(*mps.back()), {{2}, {1}});
  delete rblock1;
  rblock1 = temp_rblock1;
  rblocks[1] = rblock1;
  std::string file;
  if (sweep_params.FileIO) {
    file = kRuntimeTempPath + "/" +
               "r" + kBlockFileBaseName + "0" + "." + kGQTenFileSuffix; 
    WriteGQTensorTOFile(*rblock0, file);
    file = kRuntimeTempPath + "/" +
               "r" + kBlockFileBaseName + "1" + "." + kGQTenFileSuffix; 
    WriteGQTensorTOFile(*rblock1, file);
  }
  for (size_t i = 2; i < N-1; ++i) {
    auto rblocki = Contract(*mps[N-i], *rblocks[i-1], {{2}, {0}});
    auto temp_rblocki = Contract(*rblocki, *mpo[N-i], {{1, 2}, {1, 3}});
    delete rblocki;
    rblocki = temp_rblocki;
    temp_rblocki = Contract(*rblocki, MockDag(*mps[N-i]), {{3, 1}, {1, 2}});
    delete rblocki;
    rblocki = temp_rblocki;
    rblocks[i] = rblocki;
    if (sweep_params.FileIO) {
      auto file = kRuntimeTempPath + "/" +
                  "r" + kBlockFileBaseName + std::to_string(i) +
                  "." + kGQTenFileSuffix; 
      WriteGQTensorTOFile(*rblocki, file);
    }
  }
  if (sweep_params.FileIO) { for (auto &rb : rblocks) { delete rb; } }
  if (sweep_params.FileIO) {
    auto file = kRuntimeTempPath + "/" +
                "l" + kBlockFileBaseName + "0" + "." + kGQTenFileSuffix; 
    WriteGQTensorTOFile(GQTensor(), file);
  }
  return std::make_pair(lblocks, rblocks);
}


double TwoSiteSweep(
    std::vector<GQTensor *> &mps, const std::vector<GQTensor *> &mpo,
    std::vector<GQTensor *> &lblocks, std::vector<GQTensor *> &rblocks,
    const SweepParams &sweep_params) {
  auto N = mps.size();
  double e0;
  for (size_t i = 0; i < N-1; ++i) {
    e0 = TwoSiteUpdate(i, mps, mpo, lblocks, rblocks, sweep_params, 'r');
  }
  for (size_t i = N-1; i > 0; --i) {
    e0 = TwoSiteUpdate(i, mps, mpo, lblocks, rblocks, sweep_params, 'l');
  }
  return e0;
}


double TwoSiteUpdate(
    const long i,
    std::vector<GQTensor *> &mps, const std::vector<GQTensor *> &mpo,
    std::vector<GQTensor *> &lblocks, std::vector<GQTensor *> &rblocks,
    const SweepParams &sweep_params, const char dir) {
  Timer update_timer("update");
  update_timer.Restart();
  Timer bef_lanc_timer("bef_lanc");
  bef_lanc_timer.Restart();
  auto N = mps.size();
  std::vector<std::vector<long>> init_state_ctrct_axes, us_ctrct_axes;
  std::string where;
  long svd_ldims, svd_rdims;
  long lsite_idx, rsite_idx;
  long lblock_len, rblock_len;
  std::string lblock_file, rblock_file;
  long ee_target_site;
  bool measure_ee = false;

  switch (dir) {
    case 'r':
      lsite_idx = i;
      rsite_idx = i+1;
      lblock_len = i;
      rblock_len = N-(i+2);
      if (i == 0) {
        init_state_ctrct_axes = {{1}, {0}};
        where = "lend";
        svd_ldims = 1;
        svd_rdims = 2;
      } else if (i == N-2) {
        init_state_ctrct_axes = {{2}, {0}};
        where = "rend";
        svd_ldims = 2;
        svd_rdims = 1;
      } else {
        init_state_ctrct_axes = {{2}, {0}};
        where = "cent";
        svd_ldims = 2;
        svd_rdims = 2;
      }
      ee_target_site = sweep_params.EETargetBond;
      if (i == ee_target_site) { measure_ee = true; }
      break;
    case 'l':
      lsite_idx = i-1;
      rsite_idx = i;
      lblock_len = i-1;
      rblock_len = N-i-1;
      if (i == N-1) {
        init_state_ctrct_axes = {{2}, {0}};
        where = "rend";
        svd_ldims = 2;
        svd_rdims = 1;
        us_ctrct_axes = {{2}, {0}};
      } else if (i == 1) {
        init_state_ctrct_axes = {{1}, {0}};
        where = "lend";
        svd_ldims = 1;
        svd_rdims = 2;
        us_ctrct_axes = {{1}, {0}};
      } else {
        init_state_ctrct_axes = {{2}, {0}};
        where = "cent";
        svd_ldims = 2;
        svd_rdims = 2;
        us_ctrct_axes = {{2}, {0}};
      }
      ee_target_site = sweep_params.EETargetBond + 1;
      if (i == ee_target_site) { measure_ee = true; }
      break;
    default:
      std::cout << "dir must be 'r' or 'l', but " << dir << std::endl; 
      exit(1);
  }

  if (sweep_params.FileIO) {
    switch (dir) {
      case 'r':
        rblock_file = kRuntimeTempPath + "/" +
                      "r" + kBlockFileBaseName + std::to_string(rblock_len) +
                      "." + kGQTenFileSuffix; 
        ReadGQTensorFromFile(rblocks[rblock_len], rblock_file);
        break;
      case 'l':
        lblock_file = kRuntimeTempPath + "/" +
                      "l" + kBlockFileBaseName + std::to_string(lblock_len) +
                      "." + kGQTenFileSuffix; 
        ReadGQTensorFromFile(lblocks[lblock_len], lblock_file);
        break;
      default:
        std::cout << "dir must be 'r' or 'l', but " << dir << std::endl; 
        exit(1);
    }
  }

  auto bef_lanc_elapsed_time = bef_lanc_timer.Elapsed();
  // Lanczos
  std::vector<GQTensor *>eff_ham(4);
  eff_ham[0] = lblocks[lblock_len];
  eff_ham[1] = mpo[lsite_idx];
  eff_ham[2] = mpo[rsite_idx];
  eff_ham[3] = rblocks[rblock_len];
  auto init_state = Contract(
                        *mps[lsite_idx], *mps[rsite_idx],
                        init_state_ctrct_axes);
  Timer lancz_timer("Lancz");
  lancz_timer.Restart();
  auto lancz_res = LanczosSolver(
                       eff_ham, init_state,
                       sweep_params.LanczParams,
                       where);
  auto lancz_elapsed_time = lancz_timer.Elapsed();

  // SVD
  Timer svd_timer("svd");
  svd_timer.Restart();
  auto svd_res = Svd(
      *lancz_res.gs_vec,
      svd_ldims, svd_rdims,
      Div(*mps[lsite_idx]), Div(*mps[rsite_idx]),
      sweep_params.Cutoff,
      sweep_params.Dmin, sweep_params.Dmax);
  delete lancz_res.gs_vec;
  auto svd_elapsed_time = svd_timer.Elapsed();

  // Measure entanglement entropy.
  double ee;
  if (measure_ee) {
    ee = MeasureEE(svd_res.s, svd_res.D);
  }

  // Update MPS sites and blocks.
  Timer blk_update_timer("blkup");
  blk_update_timer.Restart();
  Timer new_blk_timer("new_blk");
  double new_blk_elapsed_time;
  Timer dump_blk_timer("dump_blk");
  double dump_blk_elapsed_time;

  GQTensor *new_lblock, *new_rblock;
  bool update_block = true;
  switch (dir) {
    case 'r':
      new_blk_timer.Restart();
      delete mps[lsite_idx];
      mps[lsite_idx] = svd_res.u;
      delete mps[rsite_idx];
      mps[rsite_idx] = Contract(*svd_res.s, *svd_res.v, {{1}, {0}});
      delete svd_res.s;
      delete svd_res.v;

      if (i == 0) {
        new_lblock = Contract(*mps[i], *mpo[i], {{0}, {0}});
        auto temp_new_lblock = Contract(
                                   *new_lblock, MockDag(*mps[i]),
                                   {{2}, {0}});
        delete new_lblock;
        new_lblock = temp_new_lblock;
      } else if (i != N-2) {
        new_lblock = Contract(*lblocks[i], *mps[i], {{0}, {0}});
        auto temp_new_lblock = Contract(*new_lblock, *mpo[i], {{0, 2}, {0, 1}});
        delete new_lblock;
        new_lblock = temp_new_lblock;
        temp_new_lblock = Contract(*new_lblock, MockDag(*mps[i]), {{0, 2}, {0, 1}});
        delete new_lblock;
        new_lblock = temp_new_lblock;
      } else {
        update_block = false;
      }
      new_blk_elapsed_time = new_blk_timer.Elapsed();

      dump_blk_timer.Restart();
      if (sweep_params.FileIO) {
        if (update_block) {
          auto target_blk_len = i+1;
          lblocks[target_blk_len] = new_lblock;
          auto target_blk_file = kRuntimeTempPath + "/" +
                                 "l" + kBlockFileBaseName + std::to_string(target_blk_len) +
                                 "." + kGQTenFileSuffix; 
          WriteGQTensorTOFile(*new_lblock, target_blk_file);
          delete eff_ham[0];
          delete eff_ham[3];
        } else {
          delete eff_ham[0];
        }
      } else {
        if (update_block) {
          auto target_blk_len = i+1;
          delete lblocks[target_blk_len];
          lblocks[target_blk_len] = new_lblock;
        }
      }
      dump_blk_elapsed_time = dump_blk_timer.Elapsed();
      break;
    case 'l':
      new_blk_timer.Restart();
      delete mps[lsite_idx];
      mps[lsite_idx] = Contract(*svd_res.u, *svd_res.s, us_ctrct_axes);
      delete svd_res.u;
      delete svd_res.s;
      delete mps[rsite_idx];
      mps[rsite_idx] = svd_res.v;

      if (i == N-1) {
        new_rblock = Contract(*mps[i], *mpo[i], {{1}, {0}});
        auto temp_new_rblock = Contract(*new_rblock, MockDag(*mps[i]), {{2}, {1}});
        delete new_rblock;
        new_rblock = temp_new_rblock;
      } else if (i != 1) {
        new_rblock = Contract(*mps[i], *eff_ham[3], {{2}, {0}});
        auto temp_new_rblock = Contract(*new_rblock, *mpo[i], {{1, 2}, {1, 3}});
        delete new_rblock;
        new_rblock = temp_new_rblock;
        temp_new_rblock = Contract(*new_rblock, MockDag(*mps[i]), {{3, 1}, {1, 2}});
        delete new_rblock;
        new_rblock = temp_new_rblock;
      } else {
        update_block = false;
      }
      new_blk_elapsed_time = new_blk_timer.Elapsed();

      dump_blk_timer.Restart();
      if (sweep_params.FileIO) {
        if (update_block) {
          auto target_blk_len = N-i;
          rblocks[target_blk_len] = new_rblock;
          auto target_blk_file = kRuntimeTempPath + "/" +
                                 "r" + kBlockFileBaseName + std::to_string(target_blk_len) +
                                 "." + kGQTenFileSuffix; 
          WriteGQTensorTOFile(*new_rblock, target_blk_file);
          delete eff_ham[0];
          delete eff_ham[3];
        } else {
          delete eff_ham[3];
        }
      } else {
        if (update_block) {
          auto target_blk_len = N-i;
          delete rblocks[target_blk_len];
          rblocks[target_blk_len] = new_rblock;
        }
      }
      dump_blk_elapsed_time = dump_blk_timer.Elapsed();
  }
  auto blk_update_elapsed_time = blk_update_timer.Elapsed();
  auto update_elapsed_time = update_timer.Elapsed();
  std::cout << "Site " << std::setw(4) << i
            << " E0 = " << std::setw(20) << std::setprecision(16) << std::fixed << lancz_res.gs_eng
            << " TruncErr = " << std::setprecision(2) << std::scientific << svd_res.trunc_err << std::fixed
            << " D = " << std::setw(5) << svd_res.D
            << " Iter = " << std::setw(3) << lancz_res.iters
            << " LanczT = " << std::setw(8) << lancz_elapsed_time
            << " TotT = " << std::setw(8) << update_elapsed_time;
  if (measure_ee) {
    std::cout << " S = " << std::setw(10) << std::setprecision(7) << ee;
  }
  std::cout << std::scientific << std::endl;
  std::cout << std::fixed << bef_lanc_elapsed_time << " " << lancz_elapsed_time << " " << svd_elapsed_time << " | " << blk_update_elapsed_time << " " << new_blk_elapsed_time << " " << dump_blk_elapsed_time << " | " << update_elapsed_time << std::scientific << std::endl;
  return lancz_res.gs_eng;
}
} /* gqmps2 */ 
