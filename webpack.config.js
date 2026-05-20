const path = require("path");
const CopyPlugin = require("copy-webpack-plugin");

module.exports = {
	entry: "./js/main.js",
	plugins: [
		new CopyPlugin({
			patterns: [
				{ from: "js/lua", to: path.resolve(__dirname, "public/lua") },
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
	mode: "development",
};
