for /R forms %%f in (*.cpp,*.h) do clang-format -i "%%f"
for /R framework %%f in (*.cpp,*.h) do clang-format -i "%%f"
for /R game %%f in (*.cpp,*.h) do clang-format -i "%%f"
for /R library %%f in (*.cpp,*.h) do clang-format -i "%%f"