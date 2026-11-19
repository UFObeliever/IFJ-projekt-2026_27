import ifj26.*

extension (v: Int)
  def label: String = "n"

@main
def main(): Unit =
  val x = 1
  val s = x.label
  if s == "n" then
    print("y")
  else
    print("n")
