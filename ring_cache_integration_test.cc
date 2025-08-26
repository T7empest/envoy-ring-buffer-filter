#include "test/integration/http_integration.h"

namespace Envoy {

	class RingCacheIntegrationTest : public testing::TestWithParam<Network::Address::IpVersion>,
									 public HttpIntegrationTest {
	public:
		RingCacheIntegrationTest()
			: HttpIntegrationTest(Http::CodecType::HTTP1, GetParam()) {}

		void SetUp() override {
			autonomous_upstream_ = true;  // fake backend
			initialize();
		}

		void TearDown() override {
			if (codec_client_ != nullptr) {
				codec_client_->close();
			}
			test_server_.reset();
			fake_upstreams_.clear();
		}
	};

	INSTANTIATE_TEST_SUITE_P(IpVersions, RingCacheIntegrationTest,
							 testing::ValuesIn(TestEnvironment::getIpVersionsForTest()));

	TEST_P(RingCacheIntegrationTest, BasicRequest)
	{
		auto conn = makeHttpConnection(lookupPort("http"));

		Http::TestRequestHeaderMapImpl headers{
	      {":method", "GET"},
		  {":path", "/"},
		  {":scheme", "http"},
		  {":authority", "host"}};

		auto response = conn->makeHeaderOnlyRequest(headers);

		EXPECT_TRUE(response->waitForEndStream());
		EXPECT_TRUE(response->complete());
		EXPECT_EQ("200", response->headers().getStatusValue());

		conn->close();
	}

	// TODO:
	TEST_P(RingCacheIntegrationTest, NonGetRequests)
	{
		auto conn = makeHttpConnection(lookupPort("http"));

		Http::TestRequestHeaderMapImpl put_request_headers{
			{":method", "PUT"}, {":path", "/foo"}, {":scheme", "http"}, {":authority", "host"}};
		auto r1 = conn->makeRequestWithBody(put_request_headers, 1);

		EXPECT_TRUE(r1->waitForEndStream());
		EXPECT_TRUE(r1->complete());
		EXPECT_THAT(r1->headers(), Http::HeaderValueOf("cache-hit-status", "cache_miss"));

		conn->close();
	}

	TEST_P(RingCacheIntegrationTest, CacheHitStatus)
	{
		auto conn = makeHttpConnection(lookupPort("http"));

		Http::TestRequestHeaderMapImpl foo_headers{
			{":method", "GET"}, {":path", "/foo"}, {":scheme", "http"}, {":authority", "host"}};
		auto r1 = conn->makeHeaderOnlyRequest(foo_headers);
		EXPECT_TRUE(r1->waitForEndStream());
		EXPECT_EQ("200", r1->headers().getStatusValue());

		// second request: similar -> should be cache_hit
		auto r2 = conn->makeHeaderOnlyRequest(foo_headers);
		EXPECT_TRUE(r2->waitForEndStream());
		EXPECT_EQ("200", r2->headers().getStatusValue());
		EXPECT_THAT(r2->headers(), Http::HeaderValueOf("cache-hit-status", "cache_hit"));

		// third request: different -> should be cache_miss
		Http::TestRequestHeaderMapImpl bar_headers{
			{":method", "GET"}, {":path", "/bar"}, {":scheme", "http"}, {":authority", "host"}};
		auto r3 = conn->makeHeaderOnlyRequest(bar_headers);
		EXPECT_TRUE(r3->waitForEndStream());
		EXPECT_EQ("200", r2->headers().getStatusValue());
		EXPECT_THAT(r2->headers(), Http::HeaderValueOf("cache-hit-status", "cache_miss"));

		conn->close();
	}

} // namespace Envoy
