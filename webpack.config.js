const path = require("path");
const CopyPlugin = require("copy-webpack-plugin");

module.exports = {
	entry: "./js/main.js",
	plugins: [
		new CopyPlugin({
			patterns: [
				{ from: "js/lua", to: path.resolve(__dirname, "public/lua") },
				// Self-hosted Lua engine: wasmoon would otherwise fetch it from unpkg.
				{ from: "node_modules/wasmoon/dist/glue.wasm", to: path.resolve(__dirname, "public/framework") },
			],
		}),
	],
	resolve: {
		fallback: {
			path: false,
			fs: false,
			child_process: false,
			crypto: false,
			url: false,
			module: false,
		},
	},
	output: {
		filename: "lua-framework.js",
		path: path.resolve(__dirname, "public/framework"),
		clean: true,
	},
	experiments: {
		topLevelAwait: true,
		asyncWebAssembly: true,
	},
	mode: "production",
};
