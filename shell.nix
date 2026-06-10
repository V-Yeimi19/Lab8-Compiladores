{
  pkgs ? import <nixpkgs> {}
}:

pkgs.mkShell {
  buildInputs = with pkgs; [
    git
    gcc
    (python312.withPackages(ps: with ps; [ rich ]))
  ];

  # Variables de entorno y scripts de inicio
  shellHook = ''

    echo "========================================================="     
    echo "Lab 7 - Compis"
    echo "========================================================="
    echo "🐍 Python:  $(python --version)"
    echo "   GCC:  $(gcc --version)"
    echo "========================================================="
  '';
}