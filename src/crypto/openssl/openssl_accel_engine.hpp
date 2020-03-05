// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
 *
 * Author: Hang Li <lihang48@huawei.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 */

#ifndef OPENSSL_ACCEL_ENGINE_H
#define OPENSSL_ACCEL_ENGINE_H

#include "common/debug.h"
#include "global/global_context.h"

#include <openssl/engine.h>

// -----------------------------------------------------------------------------
#define dout_context g_ceph_context
#define dout_subsys ceph_subsys_crypto
#undef dout_prefix
#define dout_prefix engine_prefix(_dout)

static ostream& engine_prefix(std::ostream* _dout) {
  return *_dout << "OpensslAccelEngine: ";
}
// -----------------------------------------------------------------------------

class OpensslAccelEngine {
 private:
  explicit OpensslAccelEngine(string eid) {
    if (eid != "none") {
      ENGINE_load_dynamic();

      std::unique_ptr<ENGINE, std::function<void(ENGINE*)>> engine = {
          ENGINE_by_id(eid.c_str()), [](ENGINE* e) { ENGINE_free(e); }};

      if (engine.get()) {
        if (ENGINE_init(engine.get()) == 1) {
          engine.get_deleter() = [](ENGINE* e) {
            ENGINE_finish(e);
            ENGINE_free(e);
          };
          engine_ = std::move(engine);
        } else {
          derr << "failed to init engine: " << eid << dendl;
        }
      } else {
        derr << "failed to get engine: " << eid << dendl;
      }
    }
  }

  OpensslAccelEngine(const OpensslAccelEngine&) = delete;
  OpensslAccelEngine& operator=(const OpensslAccelEngine&) = delete;

 public:
  static OpensslAccelEngine& get_instance(string eid) {
    // every type of engine could have up to one instance
    // one process may use several types of engine
    // more engines can be added here, like kae(kunpeng accelerator engine)
    if (eid == "kae") {
      static OpensslAccelEngine kae(eid);
      return kae;
    } else {
      static OpensslAccelEngine no_engine(eid);
      return no_engine;
    }
  }

  ENGINE* get_engine() { return engine_.get(); }

 private:
  std::unique_ptr<ENGINE, std::function<void(ENGINE*)>> engine_;
};

#endif
