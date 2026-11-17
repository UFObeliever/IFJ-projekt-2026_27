import ifj26.*

@main
def main(): Unit =
  val c = true
  val d = if c then 1 else 2.5
  if d == 1.0 then
    print("y")
  else
    print("n")
