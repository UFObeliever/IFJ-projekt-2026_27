import ifj26.*

def mez(n: Int): Int =
  print("m")
  n

@main def main(): Unit =
  for i <- mez(1) to mez(3) by mez(1) do print(i)
  print("\n")
