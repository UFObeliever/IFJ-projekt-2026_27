import ifj26.*

@main def main(): Unit =
  var x = 1
  if x < 2 then
    val x = 100
    print(x)
    print("\n")
  else
    print("no\n")
  x = x + 1
  print(x)
  print("\n")
