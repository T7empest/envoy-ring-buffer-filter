#include "test/integration/http_integration.h"

namespace Envoy
{
	class RingCacheIntegrationTest
		: public testing::TestWithParam<Network::Address::IpVersion>,
		  public HttpIntegrationTest
	{
	public:
		RingCacheIntegrationTest()
			: HttpIntegrationTest(Http::CodecType::HTTP1, GetParam())
		{
			setUpstreamProtocol(Http::CodecType::HTTP1);
			setUpstreamCount(1);
			autonomous_upstream_ = true;
		}

		void SetUp() override
		{
			config_helper_.addFilter(R"EOF(
name: envoy.filters.http.ring_cache
typed_config:
  "@type": type.googleapis.com/google.protobuf.Struct
  value:
    pools:
      - name: api_queries
        slots: 512
        match:
          path_prefixes:
            - "/api/"
)EOF");

			setUpstreamProtocol(FakeHttpConnection::Type::HTTP1);
			initialize();
		}

		void TearDown() override
		{
			if (codec_client_ != nullptr)
			{
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
			{":authority", "host"}
		};

		auto response = conn->makeHeaderOnlyRequest(headers);

		EXPECT_TRUE(response->waitForEndStream());
		EXPECT_TRUE(response->complete());
		EXPECT_EQ("200", response->headers().getStatusValue());

		conn->close();
	}

	//
	// Basic cache miss → hit for header-only GET
	//
	TEST_P(RingCacheIntegrationTest, CacheMissThenHit)
	{
		auto conn = makeHttpConnection(lookupPort("http"));

		const Http::TestRequestHeaderMapImpl req{
			{":method", "GET"},
			{":path", "/api/thing"},
			{":scheme", "http"},
			{":authority", "host"}
		};

		// First request: expect MISS and 200
		auto r1 = conn->makeHeaderOnlyRequest(req);
		EXPECT_TRUE(r1->waitForEndStream());
		EXPECT_TRUE(r1->complete());
		EXPECT_EQ("200", r1->headers().getStatusValue());
		EXPECT_THAT(r1->headers(), Http::HeaderValueOf("cache-hit-status", "cache_miss"));

		// Second identical request: expect HIT
		auto r2 = conn->makeHeaderOnlyRequest(req);
		EXPECT_TRUE(r2->waitForEndStream());
		EXPECT_TRUE(r2->complete());
		EXPECT_EQ("200", r2->headers().getStatusValue());
		EXPECT_THAT(r2->headers(), Http::HeaderValueOf("cache-hit-status", "cache_hit"));

		conn->close();
	}

	//
	// Different path → different cache key (second route is a MISS)
	//
	TEST_P(RingCacheIntegrationTest, DifferentPathIsDifferentKey)
	{
		auto conn = makeHttpConnection(lookupPort("http"));

		const Http::TestRequestHeaderMapImpl req_a{
			{":method", "GET"},
			{":path", "/api/a"},
			{":scheme", "http"},
			{":authority", "host"}
		};

		const Http::TestRequestHeaderMapImpl req_b{
			{":method", "GET"},
			{":path", "/api/b"},
			{":scheme", "http"},
			{":authority", "host"}
		};

		auto r1 = conn->makeHeaderOnlyRequest(req_a);
		EXPECT_TRUE(r1->waitForEndStream());
		EXPECT_TRUE(r1->complete());
		EXPECT_THAT(r1->headers(), Http::HeaderValueOf("cache-hit-status", "cache_miss"));

		auto r2 = conn->makeHeaderOnlyRequest(req_b);
		EXPECT_TRUE(r2->waitForEndStream());
		EXPECT_TRUE(r2->complete());
		// Different path => different key => MISS
		EXPECT_THAT(r2->headers(), Http::HeaderValueOf("cache-hit-status", "cache_miss"));

		// A repeat of /api/a should now HIT
		auto r3 = conn->makeHeaderOnlyRequest(req_a);
		EXPECT_TRUE(r3->waitForEndStream());
		EXPECT_TRUE(r3->complete());
		EXPECT_THAT(r3->headers(), Http::HeaderValueOf("cache-hit-status", "cache_hit"));

		conn->close();
	}
} // namespace Envoy
