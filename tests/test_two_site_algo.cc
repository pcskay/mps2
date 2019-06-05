/*
* Author: Rongyang Sun <sun-rongyang@outlook.com>
* Creation Date: 2019-05-14 16:08
* 
* Description: GraceQ/mps2 project. Unittest for two sites algorithm.
*/
#include "gqmps2/gqmps2.h"
#include "gtest/gtest.h"
#include "gqten/gqten.h"

#include <vector>


using namespace gqmps2;
using namespace gqten;


struct TestTwoSiteAlgorithmSpinSystem : public testing::Test {
  long N = 6;

  QN qn0 = QN({QNNameVal("Sz", 0)});
  Index pb_out = Index({
                     QNSector(QN({QNNameVal("Sz", 1)}), 1),
                     QNSector(QN({QNNameVal("Sz", -1)}), 1)}, OUT);
  Index pb_in = InverseIndex(pb_out);
  GQTensor sz = GQTensor({pb_in, pb_out});
  GQTensor sp = GQTensor({pb_in, pb_out});
  GQTensor sm = GQTensor({pb_in, pb_out});

  void SetUp(void) {
    sz({0, 0}) = 0.5;
    sz({1, 1}) = -0.5;
    sp({0, 1}) = 1;
    sm({1, 0}) = 1;
  }
};


TEST_F(TestTwoSiteAlgorithmSpinSystem, 1DIsing) {
  auto mpo_gen = MPOGenerator(N, pb_out, qn0);
  for (long i = 0; i < N-1; ++i) {
    mpo_gen.AddTerm(1, {OpIdx(sz, i), OpIdx(sz, i+1)});
  }
  auto mpo = mpo_gen.Gen();

  std::vector<GQTensor *> mps(N);
  srand(0); rand();   //  One more rand() called or random init state strange for Ising model.
  RandomInitMps(mps, pb_out, qn0, qn0, 2);

  auto sweep_params = SweepParams(
                          4,
                          1, 10, 1.0E-5,
                          true,
                          kTwoSiteAlgoWorkflowInitial,
                          LanczosParams(1.0E-7),
                          N/2-1);

  auto energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -0.25*(N-1), 1.0E-10);

  // No file I/O case.
  sweep_params = SweepParams(
                     2,
                     1, 10, 1.0E-5,
                     false,
                     kTwoSiteAlgoWorkflowInitial,
                     LanczosParams(1.0E-7),
                     N/2-1);

  energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -0.25*(N-1), 1.0E-10);
}


TEST_F(TestTwoSiteAlgorithmSpinSystem, 1DHeisenberg) {
  auto mpo_gen = MPOGenerator(N, pb_out, qn0);
  for (long i = 0; i < N-1; ++i) {
    mpo_gen.AddTerm(1, {OpIdx(sz, i), OpIdx(sz, i+1)});
    mpo_gen.AddTerm(0.5, {OpIdx(sp, i), OpIdx(sm, i+1)});
    mpo_gen.AddTerm(0.5, {OpIdx(sm, i), OpIdx(sp, i+1)});
  }
  auto mpo = mpo_gen.Gen();

  std::vector<GQTensor *> mps(N);
  srand(0);
  RandomInitMps(mps, pb_out, qn0, qn0, 4);

  auto sweep_params = SweepParams(
                     4,
                     8, 8, 1.0E-9,
                     true,
                     kTwoSiteAlgoWorkflowInitial,
                     LanczosParams(1.0E-7),
                     N/2-1);

  auto energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -2.493577133888, 1.0E-12);

  // Continue simulation test.
  DumpMps(mps);
  for (auto &mps_ten : mps) { delete mps_ten; }
  LoadMps(mps);

  sweep_params = SweepParams(
                     4,
                     8, 8, 1.0E-9,
                     true,
                     kTwoSiteAlgoWorkflowContinue,
                     LanczosParams(1.0E-7),
                     N/2-1);

  energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -2.493577133888, 1.0E-12);
}


TEST_F(TestTwoSiteAlgorithmSpinSystem, 2DHeisenberg) {
  auto mpo_gen = MPOGenerator(N, pb_out, qn0);
  std::vector<std::pair<long, long>> nn_pairs = {
      std::make_pair(0, 1), 
      std::make_pair(0, 2), 
      std::make_pair(1, 3), 
      std::make_pair(2, 3), 
      std::make_pair(2, 4), 
      std::make_pair(3, 5), 
      std::make_pair(4, 5)
  };
  for (auto &p : nn_pairs) {
    mpo_gen.AddTerm(1, {OpIdx(sz, p.first), OpIdx(sz, p.second)});
    mpo_gen.AddTerm(0.5, {OpIdx(sp, p.first), OpIdx(sm, p.second)});
    mpo_gen.AddTerm(0.5, {OpIdx(sm, p.first), OpIdx(sp, p.second)});
  }
  auto mpo = mpo_gen.Gen();

  std::vector<GQTensor *> mps(N);
  srand(0);
  RandomInitMps(mps, pb_out, qn0, qn0, 4);

  auto sweep_params = SweepParams(
                     4,
                     8, 8, 1.0E-9,
                     true,
                     kTwoSiteAlgoWorkflowInitial,
                     LanczosParams(1.0E-7),
                     N/2-1);

  auto energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -3.129385241572, 1.0E-10);

  // Test direct product state initialization.
  std::vector<long> stat_labs;
  for (int i = 0; i < N; ++i) { stat_labs.push_back(i % 2); }
  DirectStateInitMps(mps, stat_labs, pb_out, qn0);

  sweep_params = SweepParams(
                     4,
                     8, 8, 1.0E-9,
                     true,
                     kTwoSiteAlgoWorkflowInitial,
                     LanczosParams(1.0E-7),
                     N/2-1);

  energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -3.129385241572, 1.0E-10);
}


// Test Fermion models.
struct TestTwoSiteAlgorithmTjSystem : public testing::Test {
  long N = 4;
  double t = 3.;
  double J = 1.;
  QN qn0 = QN({QNNameVal("N", 0), QNNameVal("Sz", 0)});
  Index pb_out = Index({
      QNSector(QN({QNNameVal("N", 1), QNNameVal("Sz",  1)}), 1),
      QNSector(QN({QNNameVal("N", 1), QNNameVal("Sz", -1)}), 1),
      QNSector(QN({QNNameVal("N", 0), QNNameVal("Sz",  0)}), 1)}, OUT);
  Index pb_in = InverseIndex(pb_out);
  GQTensor f = GQTensor({pb_in, pb_out});
  GQTensor sz = GQTensor({pb_in, pb_out});
  GQTensor sp = GQTensor({pb_in, pb_out});
  GQTensor sm = GQTensor({pb_in, pb_out});
  GQTensor cup = GQTensor({pb_in, pb_out});
  GQTensor cdagup = GQTensor({pb_in, pb_out});
  GQTensor cdn = GQTensor({pb_in, pb_out});
  GQTensor cdagdn = GQTensor({pb_in, pb_out});

  void SetUp(void) {
    f({0, 0})  = -1;
    f({1, 1})  = -1;
    f({2, 2})  = 1;
    sz({0, 0}) =  0.5;
    sz({1, 1}) = -0.5;
    sp({0, 1}) = 1;
    sm({1, 0}) = 1;
    cup({2, 0}) = 1;
    cdagup({0, 2}) = 1;
    cdn({2, 1}) = 1;
    cdagdn({1, 2}) = 1;
  }
};


TEST_F(TestTwoSiteAlgorithmTjSystem, 1DCase) {
  auto mpo_gen = MPOGenerator(N, pb_out, qn0);
  for (long i = 0; i < N-1; ++i) {
    mpo_gen.AddTerm(-t, {OpIdx(cdagup, i), OpIdx(cup, i+1)});
    mpo_gen.AddTerm(-t, {OpIdx(cdagdn, i), OpIdx(cdn, i+1)});
    mpo_gen.AddTerm(-t, {OpIdx(cup, i), OpIdx(cdagup, i+1)});
    mpo_gen.AddTerm(-t, {OpIdx(cdn, i), OpIdx(cdagdn, i+1)});
    mpo_gen.AddTerm(J, {OpIdx(sz, i), OpIdx(sz, i+1)});
    mpo_gen.AddTerm(0.5*J, {OpIdx(sp, i), OpIdx(sm, i+1)});
    mpo_gen.AddTerm(0.5*J, {OpIdx(sm, i), OpIdx(sp, i+1)});
  }
  auto mpo = mpo_gen.Gen();

  std::vector<GQTensor *> mps(N);
  auto total_div = QN({QNNameVal("N", N-2), QNNameVal("Sz", 0)});
  auto zero_div = QN({QNNameVal("N", 0), QNNameVal("Sz", 0)});
  srand(0);
  RandomInitMps(mps, pb_out, total_div, zero_div, 5);

  auto sweep_params = SweepParams(
                          11,
                          8, 8, 1.0E-9,
                          true,
                          kTwoSiteAlgoWorkflowInitial,
                          LanczosParams(1.0E-8, 20),
                          N/2-1);

  auto energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -6.947478526233, 1.0E-10);
}


TEST_F(TestTwoSiteAlgorithmTjSystem, 2DCase) {
  auto mpo_gen = MPOGenerator(N, pb_out, qn0);
  std::vector<std::pair<long, long>> nn_pairs = {
      std::make_pair(0, 1), 
      std::make_pair(0, 2), 
      std::make_pair(2, 3), 
      std::make_pair(1, 3)};
  for (auto &p : nn_pairs) {
    mpo_gen.AddTerm(-t, {OpIdx(cdagup, p.first), OpIdx(cup, p.second)}, f);
    mpo_gen.AddTerm(-t, {OpIdx(cdagdn, p.first), OpIdx(cdn, p.second)}, f);
    mpo_gen.AddTerm(-t, {OpIdx(cup, p.first), OpIdx(cdagup, p.second)}, f);
    mpo_gen.AddTerm(-t, {OpIdx(cdn, p.first), OpIdx(cdagdn, p.second)}, f);
    mpo_gen.AddTerm(J, {OpIdx(sz, p.first), OpIdx(sz, p.second)});
    mpo_gen.AddTerm(0.5*J, {OpIdx(sp, p.first), OpIdx(sm, p.second)});
    mpo_gen.AddTerm(0.5*J, {OpIdx(sm, p.first), OpIdx(sp, p.second)});
  }
  auto mpo = mpo_gen.Gen();

  std::vector<GQTensor *> mps(N);
  auto total_div = QN({QNNameVal("N", N-2), QNNameVal("Sz", 0)});
  auto zero_div = QN({QNNameVal("N", 0), QNNameVal("Sz", 0)});
  srand(0);
  RandomInitMps(mps, pb_out, total_div, zero_div, 5);

  auto sweep_params = SweepParams(
                          10,
                          8, 8, 1.0E-9,
                          true,
                          kTwoSiteAlgoWorkflowInitial,
                          LanczosParams(1.0E-8, 20),
                          N/2-1);

  auto energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -8.868563739680, 1.0E-10);

  // Direct product state initialization.
  DirectStateInitMps(mps, {2, 0, 1, 2}, pb_out, zero_div);
  energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -8.868563739680, 1.0E-10);
}


struct TestTwoSiteAlgorithmHubbardSystem : public testing::Test {
  long Nx = 2;
  long Ny = 2;
  double t0 = 1.0;
  double t1 = 0.5;
  double U = 2.0;

  QN qn0 = QN({QNNameVal("Nup", 0), QNNameVal("Ndn", 0)});
  Index pb_out = Index({
      QNSector(QN({QNNameVal("Nup", 0), QNNameVal("Ndn", 0)}), 1),
      QNSector(QN({QNNameVal("Nup", 1), QNNameVal("Ndn", 0)}), 1),
      QNSector(QN({QNNameVal("Nup", 0), QNNameVal("Ndn", 1)}), 1),
      QNSector(QN({QNNameVal("Nup", 1), QNNameVal("Ndn", 1)}), 1)}, OUT);
  Index pb_in = InverseIndex(pb_out);

  GQTensor f = GQTensor({pb_in, pb_out});         // Jordan-Wigner string
  GQTensor nupdn = GQTensor({pb_in, pb_out});     // n_up*n_dn
  GQTensor adagupf = GQTensor({pb_in, pb_out});   // a^+_up*f
  GQTensor aup = GQTensor({pb_in, pb_out});
  GQTensor adagdn = GQTensor({pb_in, pb_out});
  GQTensor fadn = GQTensor({pb_in, pb_out});
  GQTensor naupf = GQTensor({pb_in, pb_out});     // -a_up*f
  GQTensor adagup = GQTensor({pb_in, pb_out});
  GQTensor nadn = GQTensor({pb_in, pb_out});
  GQTensor fadagdn = GQTensor({pb_in, pb_out});   // f*a^+_dn

  void SetUp(void) {
    f({0, 0})  = 1;
    f({1, 1})  = -1;
    f({2, 2})  = -1;
    f({3, 3})  = 1;

    nupdn({3, 3}) = 1;

    adagupf({1, 0}) = 1;
    adagupf({3, 2}) = -1;
    aup({0, 1}) = 1;
    aup({2, 3}) = 1;
    adagdn({2, 0}) = 1;
    adagdn({3, 1}) = 1;
    fadn({0, 2}) = 1;
    fadn({1, 3}) = -1;
    naupf({0, 1}) = 1;
    naupf({2, 3}) = -1;
    adagup({1, 0}) = 1;
    adagup({3, 2}) = 1;
    nadn({0, 2}) = -1;
    nadn({1, 3}) = -1;
    fadagdn({2, 0}) = -1;
    fadagdn({3, 1}) = 1;
  }
};


inline long coors2idx(
    const long x, const long y, const long Nx, const long Ny) {
  return x * Ny + y;
}


inline void KeepOrder(long &x, long &y) {
  if (x > y) {
    auto temp = y;
    y = x;
    x = temp;
  }
}


TEST_F(TestTwoSiteAlgorithmHubbardSystem, 2Dcase) {
  auto N = Nx * Ny;
  auto mpo_gen = MPOGenerator(N, pb_out, qn0);
  for (long i = 0; i < Nx; ++i) {
    for (long j = 0; j < Ny; ++j) {
      auto s0 = coors2idx(i, j, Nx, Ny);
      mpo_gen.AddTerm(U, {OpIdx(nupdn, s0)});

      if (i != Nx-1) {
        auto s1 = coors2idx(i+1, j, Nx, Ny);
        std::cout << s0 << " " << s1 << std::endl;
        mpo_gen.AddTerm(1, {OpIdx(-t0*adagupf, s0), OpIdx(aup, s1)}, f);
        mpo_gen.AddTerm(1, {OpIdx(-t0*adagdn, s0), OpIdx(fadn, s1)}, f);
        mpo_gen.AddTerm(1, {OpIdx(naupf, s0), OpIdx(-t0*adagup, s1)}, f);
        mpo_gen.AddTerm(1, {OpIdx(nadn, s0), OpIdx(-t0*fadagdn, s1)}, f);
      }
      if (j != Ny-1) {
        auto s1 = coors2idx(i, j+1, Nx, Ny);
        std::cout << s0 << " " << s1 << std::endl;
        mpo_gen.AddTerm(1, {OpIdx(-t0*adagupf, s0), OpIdx(aup, s1)}, f);
        mpo_gen.AddTerm(1, {OpIdx(-t0*adagdn, s0), OpIdx(fadn, s1)}, f);
        mpo_gen.AddTerm(1, {OpIdx(naupf, s0), OpIdx(-t0*adagup, s1)}, f);
        mpo_gen.AddTerm(1, {OpIdx(nadn, s0), OpIdx(-t0*fadagdn, s1)}, f);
      }

      if (j != Ny-1) {
        if (i != 0) {
          auto s2 = coors2idx(i-1, j+1, Nx, Ny);
          auto temp_s0 = s0;
          KeepOrder(temp_s0, s2);
          std::cout << temp_s0 << " " << s2 << std::endl;
          mpo_gen.AddTerm(1, {OpIdx(-t1*adagupf, temp_s0), OpIdx(aup, s2)}, f);
          mpo_gen.AddTerm(1, {OpIdx(-t1*adagdn, temp_s0), OpIdx(fadn, s2)}, f);
          mpo_gen.AddTerm(1, {OpIdx(naupf, temp_s0), OpIdx(-t1*adagup, s2)}, f);
          mpo_gen.AddTerm(1, {OpIdx(nadn, temp_s0), OpIdx(-t1*fadagdn, s2)}, f);
        } 
        if (i != Nx-1) {
          auto s2 = coors2idx(i+1, j+1, Nx, Ny);
          auto temp_s0 = s0;
          KeepOrder(temp_s0, s2);
          std::cout << temp_s0 << " " << s2 << std::endl;
          mpo_gen.AddTerm(1, {OpIdx(-t1*adagupf, temp_s0), OpIdx(aup, s2)}, f);
          mpo_gen.AddTerm(1, {OpIdx(-t1*adagdn, temp_s0), OpIdx(fadn, s2)}, f);
          mpo_gen.AddTerm(1, {OpIdx(naupf, temp_s0), OpIdx(-t1*adagup, s2)}, f);
          mpo_gen.AddTerm(1, {OpIdx(nadn, temp_s0), OpIdx(-t1*fadagdn, s2)}, f);
        } 
      }
    }
  }
  auto mpo = mpo_gen.Gen();

  std::vector<GQTensor *> mps(N);
  auto qn0 = QN({QNNameVal("Nup", 0), QNNameVal("Ndn", 0)});
  std::vector<long> stat_labs(N);
  for (int i = 0; i < N; ++i) { stat_labs[i] = (i % 2 == 0 ? 1 : 2); }
  DirectStateInitMps(mps, stat_labs, pb_out, qn0);
  auto sweep_params = SweepParams(
                          10,
                          16, 16, 1.0E-9,
                          true,
                          kTwoSiteAlgoWorkflowInitial,
                          LanczosParams(1.0E-8, 20),
                          N/2-1);
  auto energy0 = TwoSiteAlgorithm(mps, mpo, sweep_params);
  EXPECT_NEAR(energy0, -2.828427124746, 1.0E-10);
}
