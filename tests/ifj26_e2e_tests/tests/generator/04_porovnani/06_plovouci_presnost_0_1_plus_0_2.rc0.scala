import ifj26.*

@main def main(): Unit =
  val a = 0.1
  val b = 0.2
  val c = a + b
  val d = 0.3
  if c == d then
    print("eq")
  else
    print("ne")
  print("\n")
  if c > d then
    print("gt")
  else
    print("le")
  print("\n")
