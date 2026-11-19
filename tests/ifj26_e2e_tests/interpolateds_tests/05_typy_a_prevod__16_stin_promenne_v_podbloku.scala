import ifj26.*

@main
def main(): Unit =
  val x = 1
  if x == 1 then
    val x = 2
    print(s"$x")
  else
    print("no")
  print(s"$x")
