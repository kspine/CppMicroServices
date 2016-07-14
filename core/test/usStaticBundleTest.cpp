/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/saschazelzer/CppMicroServices/COPYRIGHT .

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/

#include <usGetBundleContext.h>
#include <usFrameworkFactory.h>
#include <usFramework.h>
#include <usBundle.h>
#include <usBundleEvent.h>
#include <usServiceEvent.h>
#include <usBundleContext.h>
#include <usBundleActivator.h>
#include <usConstants.h>

#include "usTestUtils.h"
#include "usTestUtilBundleListener.h"
#include "usTestingMacros.h"
#include "usTestingConfig.h"

using namespace us;

namespace {

// Install and start libTestBundleB and check that it exists and that the service it registers exists,
// also check that the expected events occur
void frame020a(BundleContext context, TestBundleListener& listener)
{
  auto bundleB = testing::InstallLib(context, "TestBundleB");

  US_TEST_CONDITION_REQUIRED(bundleB, "Test for existing bundle TestBundleB")

  auto bundleImportedByB = testing::GetBundle("TestBundleImportedByB", context);
  US_TEST_CONDITION_REQUIRED(bundleImportedByB, "Test for existing bundle TestBundleImportedByB")

  US_TEST_CONDITION(bundleB.GetSymbolicName() == "TestBundleB", "Test bundle name")
  US_TEST_CONDITION(bundleImportedByB.GetSymbolicName() == "TestBundleImportedByB", "Test bundle name")

  bundleB.Start();
  bundleImportedByB.Start();
  // Check if libB registered the expected service
  try
  {
    std::vector<ServiceReferenceU> refs = context.GetServiceReferences("us::TestBundleBService");
    US_TEST_CONDITION_REQUIRED(refs.size() == 2, "Test that both the service from the shared and imported library are regsitered");

    auto o1 = context.GetService(refs.front());
    US_TEST_CONDITION(o1 && !o1->empty(), "Test if first service object found");

    auto o2 = context.GetService(refs.back());
    US_TEST_CONDITION(o1 && !o2->empty(), "Test if second service object found");

    // check the listeners for events
    std::vector<BundleEvent> pEvts;
    pEvts.push_back(BundleEvent(BundleEvent::INSTALLED, bundleB));
    pEvts.push_back(BundleEvent(BundleEvent::INSTALLED, bundleImportedByB));
    pEvts.push_back(BundleEvent(BundleEvent::RESOLVED, bundleB));
    pEvts.push_back(BundleEvent(BundleEvent::STARTING, bundleB));
    pEvts.push_back(BundleEvent(BundleEvent::STARTED, bundleB));
    pEvts.push_back(BundleEvent(BundleEvent::RESOLVED, bundleImportedByB));
    pEvts.push_back(BundleEvent(BundleEvent::STARTING, bundleImportedByB));
    pEvts.push_back(BundleEvent(BundleEvent::STARTED, bundleImportedByB));

    std::vector<ServiceEvent> seEvts;
    seEvts.push_back(ServiceEvent(ServiceEvent::SERVICE_REGISTERED, refs.back()));
    seEvts.push_back(ServiceEvent(ServiceEvent::SERVICE_REGISTERED, refs.front()));

    bool relaxed = false;
  #ifndef US_BUILD_SHARED_LIBS
    relaxed = true;
  #endif
    US_TEST_CONDITION(listener.CheckListenerEvents(pEvts, seEvts, relaxed), "Test for unexpected events");

  }
  catch (const ServiceException& /*se*/)
  {
    US_TEST_FAILED_MSG(<< "test bundle, expected service not found");
  }

  US_TEST_CONDITION(bundleB.GetState() == Bundle::STATE_ACTIVE, "Test if started correctly");
}


// Stop libB and check for correct events
void frame030b(BundleContext context, TestBundleListener& listener)
{
  auto bundleB = testing::GetBundle("TestBundleB", context);
  US_TEST_CONDITION_REQUIRED(bundleB, "Test for non-null bundle")

  auto bundleImportedByB = testing::GetBundle("TestBundleImportedByB", context);
  US_TEST_CONDITION_REQUIRED(bundleImportedByB, "Test for non-null bundle")

  std::vector<ServiceReferenceU> refs
      = context.GetServiceReferences("us::TestBundleBService");
  US_TEST_CONDITION(refs.front(), "Test for first valid service reference")
  US_TEST_CONDITION(refs.back(), "Test for second valid service reference")

  try
  {
    bundleB.Stop();
    US_TEST_CONDITION(bundleB.GetState() == Bundle::STATE_RESOLVED, "Test for stopped state")
  }
  catch (const std::exception& e)
  {
    US_TEST_FAILED_MSG(<< "Stop bundle exception: " << e.what())
  }

  try
  {
    bundleImportedByB.Stop();
    US_TEST_CONDITION(bundleImportedByB.GetState() == Bundle::STATE_RESOLVED, "Test for stopped state")
  }
  catch (const std::exception& e)
  {
    US_TEST_FAILED_MSG(<< "Stop bundle exception: " << e.what())
  }

  std::vector<BundleEvent> pEvts;
  pEvts.push_back(BundleEvent(BundleEvent::STOPPING, bundleB));
  pEvts.push_back(BundleEvent(BundleEvent::STOPPED, bundleB));
  pEvts.push_back(BundleEvent(BundleEvent::STOPPING, bundleImportedByB));
  pEvts.push_back(BundleEvent(BundleEvent::STOPPED, bundleImportedByB));

  std::vector<ServiceEvent> seEvts;
  seEvts.push_back(ServiceEvent(ServiceEvent::SERVICE_UNREGISTERING, refs.back()));
  seEvts.push_back(ServiceEvent(ServiceEvent::SERVICE_UNREGISTERING, refs.front()));

  bool relaxed = false;
#ifndef US_BUILD_SHARED_LIBS
  relaxed = true;
#endif
  US_TEST_CONDITION(listener.CheckListenerEvents(pEvts, seEvts, relaxed), "Test for unexpected events");
}

// Uninstall libB and check for correct events
void frame040c(BundleContext context, TestBundleListener& listener)
{
  bool relaxed = false;
#ifndef US_BUILD_SHARED_LIBS
  relaxed = true;
#endif

    auto bundleB = testing::GetBundle("TestBundleB", context);
    US_TEST_CONDITION_REQUIRED(bundleB, "Test for non-null bundle")

    auto bundleImportedByB = testing::GetBundle("TestBundleImportedByB", context);
    US_TEST_CONDITION_REQUIRED(bundleImportedByB, "Test for non-null bundle")

    auto const bundleCount = context.GetBundles().size();
    US_TEST_CONDITION_REQUIRED(bundleCount > 0, "Test for bundle count > 0")
    bundleB.Uninstall();
    US_TEST_CONDITION(context.GetBundles().size() == bundleCount - 1, "Test for uninstall of TestBundleB")

    std::vector<BundleEvent> pEvts;
    pEvts.push_back(BundleEvent(BundleEvent::UNRESOLVED, bundleB));
    pEvts.push_back(BundleEvent(BundleEvent::UNINSTALLED, bundleB));

    US_TEST_CONDITION(listener.CheckListenerEvents(pEvts, relaxed), "Test for unexpected events");

    // Install the same lib again, we should get TestBundleB again
    auto bundles = context.InstallBundles(bundleImportedByB.GetLocation());
#ifdef US_BUILD_SHARED_LIBS
    std::size_t installCount = 2;
#else
    std::size_t installCount = bundleCount - 1; // minus system_bundle
#endif
    US_TEST_CONDITION(bundles.size() == installCount, "Test for re-install of TestBundleB")

    long oldId = bundleB.GetBundleId();
    bundleB = testing::GetBundle("TestBundleB", context);
    US_TEST_CONDITION_REQUIRED(bundleB, "Test for non-null bundle")
    US_TEST_CONDITION(oldId != bundleB.GetBundleId(), "Test for new bundle id")

    pEvts.clear();
    pEvts.push_back(BundleEvent(BundleEvent::INSTALLED, bundleB));
    US_TEST_CONDITION(listener.CheckListenerEvents(pEvts), "Test for unexpected events");

    bundleB.Uninstall();
    bundleImportedByB.Uninstall();
    US_TEST_CONDITION(context.GetBundles().size() == bundleCount - 2, "Test for uninstall of TestBundleImportedByB")

    pEvts.clear();
    pEvts.push_back(BundleEvent(BundleEvent::UNRESOLVED, bundleB));
    pEvts.push_back(BundleEvent(BundleEvent::UNINSTALLED, bundleB));
    pEvts.push_back(BundleEvent(BundleEvent::UNRESOLVED, bundleImportedByB));
    pEvts.push_back(BundleEvent(BundleEvent::UNINSTALLED, bundleImportedByB));

    US_TEST_CONDITION(listener.CheckListenerEvents(pEvts, relaxed), "Test for unexpected events");
}

} // end unnamed namespace

int usStaticBundleTest(int /*argc*/, char* /*argv*/[])
{
  US_TEST_BEGIN("StaticBundleTest");

  FrameworkFactory factory;
  std::map<std::string, Any> frameworkConfig;
  //frameworkConfig[Constants::FRAMEWORK_LOG_LEVEL] = 0;
  auto framework = factory.NewFramework(frameworkConfig);
  framework.Start();

  auto context = framework.GetBundleContext();

  { // scope the use of the listener so its destructor is
    // called before we destroy the framework's bundle context.
    // The TestBundleListener needs to remove its listeners while
    // the framework is still active.
    TestBundleListener listener;

    BundleListenerRegistrationHelper<TestBundleListener> ml(context, &listener, &TestBundleListener::BundleChanged);
    ServiceListenerRegistrationHelper<TestBundleListener> sl(context, &listener, &TestBundleListener::ServiceChanged);

    frame020a(context, listener);
    frame030b(context, listener);
    frame040c(context, listener);
  }

  US_TEST_END()
}
