# Version
haskey(ENV, "UNO_RELEASE") || error("The environment variable UNO_RELEASE is not defined.")
version = VersionNumber(ENV["UNO_RELEASE"])
version2 = ENV["UNO_RELEASE"]
package = "Uno"

platforms = [
   ("aarch64-apple-darwin-cxx11"  , "lib", "dylib"),
#  ("aarch64-linux-gnu-cxx11"     , "lib", "so"   ),
#  ("aarch64-linux-musl-cxx11"    , "lib", "so"   ),
#  ("powerpc64le-linux-gnu-cxx11" , "lib", "so"   ),
   ("x86_64-apple-darwin-cxx11"   , "lib", "dylib"),
   ("x86_64-linux-gnu-cxx11"      , "lib", "so"   ),
#  ("x86_64-linux-musl-cxx11"     , "lib", "so"   ),
#  ("x86_64-unknown-freebsd-cxx11", "lib", "so"   ),
   ("x86_64-w64-mingw32-cxx11"    , "bin", "dll"  ),
]

for (platform, libdir, ext) in platforms

  tarball_name = "$package.v$version.$platform.tar.gz"

  if isfile("products/$(tarball_name)")
    # Unzip the tarball generated by BinaryBuilder.jl
    isdir("products/$platform") && rm("products/$platform", recursive=true)
    mkdir("products/$platform")
    run(`tar -xzf products/$(tarball_name) -C products/$platform`)

    if isfile("products/$platform/deps.tar.gz")
      # Unzip the tarball of the dependencies
      run(`tar -xzf products/$platform/deps.tar.gz -C products/$platform`)

      # Copy the license of each dependency
      display("products/$platform/deps/licenses" |> readdir)
      for folder in readdir("products/$platform/deps/licenses")
        dipslay(folder)
        cp("products/$platform/deps/licenses/$folder", "products/$platform/share/licenses/$folder")
      end
      rm("products/$platform/deps/licenses", recursive=true)

      # Copy the shared library of each dependency
      for file in readdir("products/$platform/deps")
        cp("products/$platform/deps/$file", "products/$platform/$libdir/$file")
      end

      # Remove the folder used to unzip the tarball of the dependencies
      rm("products/$platform/deps", recursive=true)
      rm("products/$platform/deps.tar.gz", recursive=true)

      # Create the archives *_binaries
      isfile("$(package)_binaries.$version2.$platform.tar.gz") && rm("$(package)_binaries.$version2.$platform.tar.gz")
      isfile("$(package)_binaries.$version2.$platform.zip") && rm("$(package)_binaries.$version2.$platform.zip")
      cd("products/$platform")

      # Create a folder with the version number of the package
      mkdir("$(package)_binaries.$version2")
      for folder in ("lib", "bin", "share")
        cp(folder, "$(package)_binaries.$version2/$folder")
      end

      cd("$(package)_binaries.$version2")
      if ext == "dll"
        run(`zip -r --symlinks ../../../$(package)_binaries.$version2.$platform.zip lib bin`)
      else
        run(`tar -czf ../../../$(package)_binaries.$version2.$platform.tar.gz lib bin`)
      end
      cd("../../..")

      # Remove the folder used to unzip the tarball generated by BinaryBuilder.jl
      rm("products/$platform", recursive=true)
    else
      @warn("The tarball deps.tar.gz is missing in $(tarball_name)!")
    end
  else
    @warn("The tarball for the platform $platform was not generated!")
  end
end
