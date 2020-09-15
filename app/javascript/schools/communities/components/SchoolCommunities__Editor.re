let str = React.string;

open SchoolCommunities__IndexTypes;

module CreateCommunityQuery = [%graphql
  {|
  mutation CreateCommunityMutation($name: String!, $targetLinkable: Boolean!, $courseIds: [ID!]!, $topicCategories: [String!]!) {
    createCommunity(name: $name, targetLinkable: $targetLinkable, courseIds: $courseIds, topicCategories: $topicCategories ) {
      community {
        id
        name
        targetLinkable
        topicCategories {
          id
          name
          topicsCount
        }
      }
    }
  }
|}
];

module UpdateCommunityQuery = [%graphql
  {|
  mutation UpdateCommunityMutation($id: ID!, $name: String!, $targetLinkable: Boolean!, $courseIds: [ID!]!) {
    updateCommunity(id: $id, name: $name, targetLinkable: $targetLinkable, courseIds: $courseIds)  {
      communityId
    }
  }
|}
];

type state = {
  saving: bool,
  dirty: bool,
  name: string,
  targetLinkable: bool,
  selectedCourseIds: Belt.Set.String.t,
  courseSearch: string,
};

let computeInitialState = ((community, connections)) => {
  let (name, targetLinkable, selectedCourseIds) =
    switch (community) {
    | Some(community) => (
        community |> Community.name,
        community |> Community.targetLinkable,
        connections
        |> List.filter(connection =>
             connection
             |> Connection.communityId == (community |> Community.id)
           )
        |> List.map(connection => connection |> Connection.courseId)
        |> Array.of_list
        |> Belt.Set.String.fromArray,
      )
    | None => ("", false, Belt.Set.String.empty)
    };

  {
    saving: false,
    dirty: false,
    name,
    targetLinkable,
    selectedCourseIds,
    courseSearch: "",
  };
};

type action =
  | UpdateName(string)
  | SetTargetLinkable(bool)
  | SelectCourse(string)
  | DeselectCourse(string)
  | BeginSaving
  | FailSaving
  | FinishSaving
  | UpdateCourseSearch(string);

let reducer = (state, action) =>
  switch (action) {
  | UpdateName(name) => {...state, name, dirty: true}
  | SetTargetLinkable(targetLinkable) => {
      ...state,
      targetLinkable,
      dirty: true,
    }
  | SelectCourse(courseId) => {
      ...state,
      dirty: true,
      selectedCourseIds:
        state.selectedCourseIds->Belt.Set.String.add(courseId),
    }
  | DeselectCourse(courseId) => {
      ...state,
      dirty: true,
      selectedCourseIds:
        state.selectedCourseIds->Belt.Set.String.remove(courseId),
    }
  | BeginSaving => {...state, saving: true}
  | FailSaving => {...state, saving: false}
  | FinishSaving => {...state, saving: false, dirty: false}
  | UpdateCourseSearch(courseSearch) => {...state, courseSearch}
  };

let handleConnections = (communityId, connections, courseIds) => {
  let oldConnections =
    connections
    |> List.filter(connection =>
         connection |> Connection.communityId != communityId
       );
  let newConnectionsForCommunity =
    courseIds
    |> Array.map(courseId => Connection.create(communityId, courseId))
    |> Array.to_list;
  oldConnections |> List.append(newConnectionsForCommunity);
};

let handleQuery =
    (
      community,
      connections,
      state,
      send,
      addCommunityCB,
      updateCommunitiesCB,
      event,
    ) => {
  event |> ReactEvent.Mouse.preventDefault;

  let {name, targetLinkable} = state;
  let courseIds = state.selectedCourseIds |> Belt.Set.String.toArray;

  if (name != "") {
    send(BeginSaving);

    switch (community) {
    | Some(community) =>
      UpdateCommunityQuery.make(
        ~id=community |> Community.id,
        ~name,
        ~targetLinkable,
        ~courseIds,
        (),
      )
      |> GraphqlQuery.sendQuery
      |> Js.Promise.then_(response => {
           let communityId = response##updateCommunity##communityId;
           switch (communityId) {
           | Some(id) =>
             updateCommunitiesCB(
               Community.create(id, name, targetLinkable, [||]),
               handleConnections(id, connections, courseIds),
             );
             send(FinishSaving);
           | None => send(FinishSaving)
           };

           Notification.success("Success", "Community updated successfully.");
           Js.Promise.resolve();
         })
      |> ignore
    | None =>
      CreateCommunityQuery.make(
        ~name,
        ~targetLinkable,
        ~courseIds,
        ~topicCategories=[|"test category"|],
        (),
      )
      |> GraphqlQuery.sendQuery
      |> Js.Promise.then_(response => {
           switch (response##createCommunity##community) {
           | Some(community) =>
             addCommunityCB(
               Community.makeFromJs(community),
               handleConnections(community##id, connections, courseIds),
             );
             send(FinishSaving);
           | None => send(FinishSaving)
           };
           Js.Promise.resolve();
         })
      |> Js.Promise.catch(error => {
           Js.log(error);
           Notification.error(
             "Unexpected Error!",
             "Please reload the page before trying to post again.",
           );
           Js.Promise.resolve();
         })
      |> ignore
    };
  } else {
    Notification.error("Empty", "Answer cant be blank");
  };
};

let booleanButtonClasses = bool => {
  let classes = "toggle-button__button";
  classes ++ (bool ? " toggle-button__button--active" : "");
};

module Selectable = {
  type t = Course.t;
  let value = t => t |> Course.name;
  let searchString = value;
};

module CourseSelector = MultiselectInline.Make(Selectable);

let selectedCourses = (~invert=false, courses, selectedCourseIds) =>
  courses
  |> Array.of_list
  |> Js.Array.filter(course => {
       let condition =
         selectedCourseIds->Belt.Set.String.has(course |> Course.id);
       invert ? !condition : condition;
     });

let unselectedCourses = (courses, selectedCourseIds) => {
  selectedCourses(~invert=true, courses, selectedCourseIds);
};

let onChangeCourseSearch = (send, value) => send(UpdateCourseSearch(value));

let onSelectCourse = (send, course) =>
  send(SelectCourse(course |> Course.id));

let onDeselectCourse = (send, course) =>
  send(DeselectCourse(course |> Course.id));

let categoryList = categories => {
  categories
  |> Js.Array.map(category =>
       <div
         key={category |> Category.id}
         className="flex justify-between bg-white-100 border shadow rounded-lg mt-2 px-2">
         <div className="flex flex-col flex-1 flex-wrap p-3">
           <div className="flex items-center">
             <div className="mr-1 font-semibold">
               {category |> Category.name |> str}
             </div>
           </div>
         </div>
         <div>
           <span className="bg-gray-300 py-1 px-2">
             {category |> Category.topicsCount |> string_of_int |> str}
           </span>
           <button
             className="p-3 text-gray-700 hover:text-gray-900 hover:bg-gray-100">
             <i className="fas fa-trash-alt" />
           </button>
         </div>
       </div>
     )
  |> React.array;
};

[@react.component]
let make =
    (
      ~courses,
      ~community,
      ~connections,
      ~addCommunityCB,
      ~categories,
      ~updateCommunitiesCB,
    ) => {
  let (state, send) =
    React.useReducerWithMapState(
      reducer,
      (community, connections),
      computeInitialState,
    );

  let saveDisabled = state.name |> String.trim == "" || !state.dirty;

  <div className="mx-8 pt-8">
    <h5 className="uppercase text-center border-b border-gray-400 pb-2">
      {"Community Editor" |> str}
    </h5>
    <DisablingCover disabled={state.saving}>
      <div key="communities-editor" className="mt-3">
        <div className="mt-2">
          <label
            className="inline-block tracking-wide text-gray-700 text-xs font-semibold"
            htmlFor="communities-editor__name">
            {"What do you want to call this community?" |> str}
          </label>
          <input
            placeholder="This community needs a name!"
            value={state.name}
            onChange={event => {
              let name = ReactEvent.Form.target(event)##value;
              send(UpdateName(name));
            }}
            id="communities-editor__name"
            className="appearance-none h-10 mt-2 block w-full text-gray-700 border border-gray-400 rounded py-2 px-4 text-sm bg-gray-100 hover:bg-gray-200 focus:outline-none focus:bg-white focus:border-primary-400"
          />
          <School__InputGroupError
            message="is not a valid name"
            active={state.dirty ? state.name |> String.trim == "" : false}
          />
        </div>
        <div className="flex items-center mt-6">
          <label
            className="inline-block tracking-wide text-gray-700 text-xs font-semibold"
            htmlFor="communities-editor__course-list">
            {"Should students be allowed to discuss targets in this community?"
             |> str}
          </label>
          <div
            className="flex toggle-button__group flex-no-shrink rounded-lg overflow-hidden ml-2">
            <button
              onClick={_ => {send(SetTargetLinkable(true))}}
              className={booleanButtonClasses(state.targetLinkable)}>
              {"Yes" |> str}
            </button>
            <button
              onClick={_ => {send(SetTargetLinkable(false))}}
              className={booleanButtonClasses(!state.targetLinkable)}>
              {"No" |> str}
            </button>
          </div>
        </div>
        <div className="mt-4">
          <label
            className="inline-block tracking-wide text-gray-700 text-xs font-semibold mb-2"
            htmlFor="communities-editor__course-targetLinkable">
            {"Give access to students from:" |> str}
          </label>
          <CourseSelector
            placeholder="Search for a course"
            emptySelectionMessage="No courses selected"
            allItemsSelectedMessage="You have selected all available courses"
            selected={selectedCourses(courses, state.selectedCourseIds)}
            unselected={unselectedCourses(courses, state.selectedCourseIds)}
            onChange={onChangeCourseSearch(send)}
            value={state.courseSearch}
            onSelect={onSelectCourse(send)}
            onDeselect={onDeselectCourse(send)}
          />
        </div>
        <div className="mt-4">
          <label
            className="inline-block tracking-wide text-gray-700 text-xs font-semibold mb-2"
            htmlFor="communities-editor__course-targetLinkable">
            {"Manage Topic Categories:" |> str}
          </label>
          {categoryList(categories)}
        </div>
      </div>
      <button
        disabled=saveDisabled
        onClick={handleQuery(
          community,
          connections,
          state,
          send,
          addCommunityCB,
          updateCommunitiesCB,
        )}
        key="communities-editor__update-button"
        className="w-full btn btn-large btn-primary mt-3">
        {(
           switch (community) {
           | Some(_) => "Update Community"
           | None => "Create a new community"
           }
         )
         |> str}
      </button>
    </DisablingCover>
    <div className="mt-3 mb-3 text-xs">
      <span className="leading-normal">
        <strong> {"Note:" |> str} </strong>
        {" Coaches in your school have access to all communities." |> str}
      </span>
    </div>
  </div>;
};
