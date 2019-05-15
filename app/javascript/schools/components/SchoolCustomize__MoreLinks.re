let str = ReasonReact.string;

type state = bool;

let component = ReasonReact.reducerComponent("SchoolCustomize__MoreLinks");

let toggleState = (send, event) => {
  event |> ReactEvent.Mouse.preventDefault;
  send();
};

let additionalLinks = (linksVisible, links) =>
  if (linksVisible) {
    <div
      className="border-2 border-grey-200 rounded-lg absolute w-48 bg-white mt-2">
      {
        links
        |> List.map(((id, title, _)) =>
             <div key=id className="p-2 cursor-default">
               <span> {title |> str} </span>
             </div>
           )
        |> Array.of_list
        |> ReasonReact.array
      }
    </div>;
  } else {
    ReasonReact.null;
  };

let make = (~links, _children) => {
  ...component,
  initialState: () => false,
  reducer: (_action, linksVisible) => ReasonReact.Update(!linksVisible),
  render: ({state, send}) =>
    switch (links) {
    | [] => ReasonReact.null
    | moreLinks =>
      <div
        title="Show more links"
        className="ml-6 font-semibold text-sm cursor-pointer relative"
        onClick={toggleState(send)}
        key="more-links">
        <span> {"More" |> str} </span>
        <i className="fas fa-angle-down ml-1" />
        {additionalLinks(state, moreLinks)}
      </div>
    },
};