import ifj26.*

@main def main(): Unit =
  val x = 1
  if x < 2 then
    val x = "inner"
    print(x)
    print("\n")
  else
    print("no\n")
  print(x)
  print("\n")
