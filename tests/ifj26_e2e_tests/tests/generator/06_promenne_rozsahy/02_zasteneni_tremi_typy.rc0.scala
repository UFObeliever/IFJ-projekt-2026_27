import ifj26.*

@main def main(): Unit =
  val x = 1
  if x < 2 then
    val x = 2.5
    if x > 1.0 then
      val x = "deep"
      print(x)
      print("\n")
    else
      print("no\n")
    print(x)
    print("\n")
  else
    print("no\n")
  print(x)
  print("\n")
