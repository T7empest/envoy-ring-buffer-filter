# HTTP response caching, request coalescing patch for Envoy

This project is based on `envoy-filter-example` and demonstrates basic response caching
in ring-buffers and coalescing of multiple requests heading towards origin at the same time.


## Building
1. `$ git clone --recurse-submodules https://github.com/T7empest/envoy-ring-buffer-filter`
2. `$ cd envoy-ring-buffer-filter`
3. `$ bazel build //:envoy`

## Configuration
You can configure the cache in `ring_cache_server.yaml` under HTTP filters >> pools

## Testing

To run the integration tests:

`$ bazel test //:ring_cache_integration_test --test_output=all`

## Running

Run script:

`$ ./run_envoy.sh`

Runtime test (sends multiple requests)

`$ python3 test/send_requests.py`
## How it works

The [Envoy repository](https://github.com/envoyproxy/envoy/) is provided as a submodule.
The [`WORKSPACE`](WORKSPACE) file maps the `@envoy` repository to this local path.

The [`BUILD`](BUILD) file introduces a new Envoy static binary target, `envoy`,
that links together the new filter and `@envoy//source/exe:envoy_main_entry_lib`. The
`echo2` filter registers itself during the static initialization phase of the
Envoy binary as a new filter.
