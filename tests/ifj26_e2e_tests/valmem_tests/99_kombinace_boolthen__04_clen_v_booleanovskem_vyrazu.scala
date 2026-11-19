import ifj26.*

@main
def main(): Unit =
  val s = "abc"
  val d = 3.5
  if s.length > 2 && d.toInt == 3 then
    print("y")
  else
    print("n")
