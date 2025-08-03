{
	description = "<project name potukange>";
	inputs = {
		nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
		flake-utils.url = "github:numtide/flake-utils";
	};
	outputs = { self, nixpkgs, flake-utils }:
	    flake-utils.lib.eachDefaultSystem (system:
	    	let
	      		pkgs = nixpkgs.legacyPackages.${system};
	      	in {
	      		devShells.default = pkgs.mkShell {
	      		 	packages = with pkgs; [
	      		 		clang
	      		 		gnumake
	      		 		sdl3
						sdl3-ttf
	      		 		pkg-config
	      		 	];
	      		};
	      	}
	    ) // {
	      templates.default = {
	        path = ./.;
	      };
	    };
}
